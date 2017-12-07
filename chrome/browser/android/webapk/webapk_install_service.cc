// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/webapk/webapk_install_service.h"

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/bind.h"
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
class WebApkInstallSpaceManager
    : public content::BrowsingDataRemover::Observer {
 public:
  WebApkInstallSpaceManager() : space_status_(SpaceStatus::UNDERDETERMINED) {}

  ~WebApkInstallSpaceManager() override {}

  // Return whether we know for sure there is no enough space to install.
  bool EnoughSpaceToInstall() { return space_status_ != NOT_ENOUGH_SPACE; }

  // Run the install callback. Free cache if there is no enough space before
  // installing.
  void InstallAndFreeCacheIfNecessary(content::BrowserContext* context,
                                      std::function<void()> callback) {
    context_ = context;
    install_callback_ = callback;

    if (space_status_ != ENOUGH_SPACE_AFTER_FREE_UP_CACHE) {
      install_callback_();
      return;
    }
    remover_ = content::BrowserContext::GetBrowsingDataRemover(context_);

    remover_->AddObserver(this);
    int cache_mask = content::BrowsingDataRemover::DATA_TYPE_CACHE_STORAGE |
                     content::BrowsingDataRemover::DATA_TYPE_CACHE;
    remover_->RemoveAndReply(
        base::Time(), base::Time::Max(), cache_mask,
        ChromeBrowsingDataRemoverDelegate::ALL_ORIGIN_TYPES, this);
  }

  // Set the space status.
  void SetSpaceStatus(int status) { space_status_ = status; }

 private:
  void OnBrowsingDataRemoverDone() override {
    remover_->RemoveObserver(this);
    install_callback_();
  }

  int space_status_;

  content::BrowserContext* context_;  // do not own

  std::function<void()> install_callback_;

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
      space_manager_(nullptr),
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

  if (space_manager_ == nullptr) {
    TriggerFreeSpaceCheck();
  }

  if (!space_manager_->EnoughSpaceToInstall()) {
    ShortcutHelper::AddToLauncherWithSkBitmap(web_contents, shortcut_info,
                                              primary_icon);
    return;
  }

  installs_.insert(shortcut_info.manifest_url);
  InstallableMetrics::TrackInstallSource(install_source);

  ShowInstallInProgressNotification(shortcut_info, primary_icon);

  space_manager_->InstallAndFreeCacheIfNecessary(
      browser_context_,
      [this, web_contents, shortcut_info, primary_icon, badge_icon]() {
        // We pass an observer which wraps the WebContents to the callback,
        // since the installation may take more than 10 seconds so there is a
        // chance that the WebContents has been destroyed before the install is
        // finished.
        auto observer = base::MakeUnique<LifetimeObserver>(web_contents);
        WebApkInstaller::InstallAsync(
            browser_context_, shortcut_info, primary_icon, badge_icon,
            base::Bind(&WebApkInstallService::OnFinishedInstall,
                       weak_ptr_factory_.GetWeakPtr(), base::Passed(&observer),
                       shortcut_info, primary_icon));
      });
}

void WebApkInstallService::UpdateAsync(
    const base::FilePath& update_request_path,
    const FinishCallback& finish_callback) {
  WebApkInstaller::UpdateAsync(browser_context_, update_request_path,
                               finish_callback);
}

void WebApkInstallService::TriggerFreeSpaceCheck() {
  space_manager_ = new WebApkInstallSpaceManager();
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_WebApkInstallService_checkFreeSpaceAndSetStatus(env, (long)this);
}

void WebApkInstallService::SetSpaceStatus(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& obj,
    int status) {
  space_manager_->SetSpaceStatus(status);
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
