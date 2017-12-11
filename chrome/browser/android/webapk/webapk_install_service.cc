// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/webapk/webapk_install_service.h"

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/bind.h"
#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/memory/ptr_util.h"
#include "chrome/browser/android/shortcut_helper.h"
#include "chrome/browser/android/shortcut_info.h"
#include "chrome/browser/android/webapk/webapk_install_service_factory.h"
#include "chrome/browser/android/webapk/webapk_installer.h"
#include "chrome/browser/browsing_data/chrome_browsing_data_remover_delegate.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browsing_data_remover.h"
#include "jni/WebApkInstallService_jni.h"
#include "ui/gfx/android/java_bitmap.h"

namespace {
class CacheClearer : public content::BrowsingDataRemover::Observer {
 public:
  CacheClearer() {}

  ~CacheClearer() override {}

  // Clear Chrome's cache. Run |callback| once clearing the cache is complete.
  void FreeCacheAsync(content::BrowsingDataRemover* remover,
                      base::OnceClosure callback) {
    install_callback_ = std::move(callback);
    remover_ = remover;
    remover_->AddObserver(this);
    remover_->RemoveAndReply(
        base::Time(), base::Time::Max(),
        content::BrowsingDataRemover::DATA_TYPE_CACHE,
        ChromeBrowsingDataRemoverDelegate::ALL_ORIGIN_TYPES, this);
  }

 private:
  void OnBrowsingDataRemoverDone() override {
    std::move(install_callback_).Run();
    remover_->RemoveObserver(this);
    delete this;
  }

  base::OnceClosure install_callback_;

  content::BrowsingDataRemover* remover_;
};

}  // namespace

// static
WebApkInstallService* WebApkInstallService::Get(
    content::BrowserContext* context) {
  return WebApkInstallServiceFactory::GetForBrowserContext(context);
}

WebApkInstallService::WebApkInstallService(
    content::BrowserContext* browser_context)
    : browser_context_(browser_context),
      space_status_(SpaceStatus::UNDERDETERMINED),
      weak_ptr_factory_(this) {}

WebApkInstallService::~WebApkInstallService() {}

bool WebApkInstallService::IsInstallInProgress(const GURL& web_manifest_url) {
  return installs_.count(web_manifest_url);
}

void WebApkInstallService::InstallAsync(content::WebContents* web_contents,
                                        const ShortcutInfo& shortcut_info,
                                        const SkBitmap& primary_icon,
                                        const SkBitmap& badge_icon,
                                        WebAppInstallSource install_source) {
  if (IsInstallInProgress(shortcut_info.manifest_url)) {
    ShortcutHelper::ShowWebApkInstallInProgressToast();
    return;
  }

  installs_.insert(shortcut_info.manifest_url);
  InstallableMetrics::TrackInstallSource(install_source);

  ShowInstallInProgressNotification(shortcut_info, primary_icon);

  auto observer = std::make_unique<LifetimeObserver>(web_contents);

  if (space_status_ == NOT_ENOUGH_SPACE) {
    OnFinishedInstall(std::move(observer), shortcut_info, primary_icon,
                      WebApkInstallResult::FAILURE, false, "");
    return;
  }

  FinishCallback finish_callback = base::Bind(
      &WebApkInstallService::OnFinishedInstall, weak_ptr_factory_.GetWeakPtr(),
      base::Passed(&observer), shortcut_info, primary_icon);

  base::OnceClosure freed_cache_callback =
      base::BindOnce(&WebApkInstaller::InstallAsync, browser_context_,
                     shortcut_info, primary_icon, badge_icon, finish_callback);

  if (space_status_ == ENOUGH_SPACE_AFTER_FREE_UP_CACHE) {
    auto* cache_cleaner = new CacheClearer();  // delete itself once done
    cache_cleaner->FreeCacheAsync(
        content::BrowserContext::GetBrowsingDataRemover(browser_context_),
        std::move(freed_cache_callback));
    return;
  } else {
    std::move(freed_cache_callback).Run();
  }
}

void WebApkInstallService::UpdateAsync(
    const base::FilePath& update_request_path,
    const FinishCallback& finish_callback) {
  WebApkInstaller::UpdateAsync(browser_context_, update_request_path,
                               finish_callback);
}

void WebApkInstallService::TriggerFreeSpaceCheck() {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::Callback<void(int)>* check_space_callback =
      new base::Callback<void(int)>(
          base::Bind([](base::WeakPtr<WebApkInstallService> service,
                        int status) { service->space_status_ = status; },
                     weak_ptr_factory_.GetWeakPtr()));
  uintptr_t callback_pointer =
      reinterpret_cast<uintptr_t>(check_space_callback);
  Java_WebApkInstallService_checkFreeSpace(env, callback_pointer);
}

void JNI_WebApkInstallService_OnGetSpaceStatus(
    JNIEnv* env,
    const base::android::JavaParamRef<jclass>& clazz,
    int status,
    jlong jcheck_space_callback) {
  base::Callback<void(int)>* check_space_callback =
      reinterpret_cast<base::Callback<void(int)>*>(jcheck_space_callback);
  check_space_callback->Run(status);
  delete check_space_callback;
}

void WebApkInstallService::OnFinishedInstall(
    std::unique_ptr<LifetimeObserver> observer,
    const ShortcutInfo& shortcut_info,
    const SkBitmap& primary_icon,
    WebApkInstallResult result,
    bool relax_updates,
    const std::string& webapk_package_name) {
  installs_.erase(shortcut_info.manifest_url);

  if (result == WebApkInstallResult::SUCCESS) {
    ShowInstalledNotification(shortcut_info, primary_icon, webapk_package_name);
    return;
  }

  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedJavaLocalRef<jstring> java_manifest_url =
      base::android::ConvertUTF8ToJavaString(env,
                                             shortcut_info.manifest_url.spec());
  Java_WebApkInstallService_cancelNotification(env, java_manifest_url);

  // If the install didn't definitely fail, we don't add a shortcut. This could
  // happen if Play was busy with another install and this one is still queued
  // (and hence might succeed in the future).
  if (result == WebApkInstallResult::FAILURE) {
    content::WebContents* web_contents = observer->web_contents();
    if (!web_contents)
      return;

    ShortcutHelper::AddToLauncherWithSkBitmap(web_contents, shortcut_info,
                                              primary_icon);
  }
}

// static
void WebApkInstallService::ShowInstallInProgressNotification(
    const ShortcutInfo& shortcut_info,
    const SkBitmap& primary_icon) {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedJavaLocalRef<jstring> java_manifest_url =
      base::android::ConvertUTF8ToJavaString(env,
                                             shortcut_info.manifest_url.spec());
  base::android::ScopedJavaLocalRef<jstring> java_short_name =
      base::android::ConvertUTF16ToJavaString(env, shortcut_info.short_name);
  base::android::ScopedJavaLocalRef<jstring> java_url =
      base::android::ConvertUTF8ToJavaString(env, shortcut_info.url.spec());
  base::android::ScopedJavaLocalRef<jobject> java_primary_icon =
      gfx::ConvertToJavaBitmap(&primary_icon);

  Java_WebApkInstallService_showInstallInProgressNotification(
      env, java_manifest_url, java_short_name, java_url, java_primary_icon);
}

// static
void WebApkInstallService::ShowInstalledNotification(
    const ShortcutInfo& shortcut_info,
    const SkBitmap& primary_icon,
    const std::string& webapk_package_name) {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedJavaLocalRef<jstring> java_webapk_package =
      base::android::ConvertUTF8ToJavaString(env, webapk_package_name);
  base::android::ScopedJavaLocalRef<jstring> java_manifest_url =
      base::android::ConvertUTF8ToJavaString(env,
                                             shortcut_info.manifest_url.spec());
  base::android::ScopedJavaLocalRef<jstring> java_short_name =
      base::android::ConvertUTF16ToJavaString(env, shortcut_info.short_name);
  base::android::ScopedJavaLocalRef<jstring> java_url =
      base::android::ConvertUTF8ToJavaString(env, shortcut_info.url.spec());
  base::android::ScopedJavaLocalRef<jobject> java_primary_icon =
      gfx::ConvertToJavaBitmap(&primary_icon);

  Java_WebApkInstallService_showInstalledNotification(
      env, java_webapk_package, java_manifest_url, java_short_name, java_url,
      java_primary_icon);
}
