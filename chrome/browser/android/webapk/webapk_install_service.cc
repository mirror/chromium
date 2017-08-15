// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/webapk/webapk_install_service.h"

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "chrome/browser/android/shortcut_helper.h"
#include "chrome/browser/android/shortcut_info.h"
#include "chrome/browser/android/webapk/webapk_install_service_factory.h"
#include "chrome/browser/android/webapk/webapk_installer.h"
#include "jni/WebApkInstallService_jni.h"
#include "ui/gfx/android/java_bitmap.h"

// static
WebApkInstallService* WebApkInstallService::Get(
    content::BrowserContext* context) {
  return WebApkInstallServiceFactory::GetForBrowserContext(context);
}

WebApkInstallService::WebApkInstallService(
    content::BrowserContext* browser_context)
    : browser_context_(browser_context),
      weak_ptr_factory_(this) {}

WebApkInstallService::~WebApkInstallService() {}

bool WebApkInstallService::IsInstallInProgress(const GURL& web_manifest_url) {
  return installs_.count(web_manifest_url);
}

void WebApkInstallService::InstallAsync(content::WebContents* web_contents,
                                        const ShortcutInfo& shortcut_info,
                                        const SkBitmap& primary_icon,
                                        const SkBitmap& badge_icon,
                                        webapk::InstallSource install_source) {
  if (IsInstallInProgress(shortcut_info.manifest_url)) {
    ShortcutHelper::ShowWebApkInstallInProgressToast();
    return;
  }

  webapk::TrackInstallSource(install_source);
  installs_.insert(shortcut_info.manifest_url);

  ShowInstallNotification(shortcut_info, primary_icon, std::string());

  auto observer = base::MakeUnique<Observer>(web_contents);
  WebApkInstaller::InstallAsync(
      browser_context_, shortcut_info, primary_icon, badge_icon,
      base::Bind(&WebApkInstallService::OnFinishedInstall,
                 weak_ptr_factory_.GetWeakPtr(), base::Passed(&observer),
                 shortcut_info, primary_icon));
}

void WebApkInstallService::UpdateAsync(
    const std::string& webapk_package,
    const GURL& start_url,
    const base::string16& short_name,
    std::unique_ptr<std::vector<uint8_t>> serialized_proto,
    const FinishCallback& finish_callback) {
  WebApkInstaller::UpdateAsync(browser_context_, webapk_package, start_url,
                               short_name, std::move(serialized_proto),
                               finish_callback);
}

void WebApkInstallService::OnFinishedInstall(
    std::unique_ptr<Observer> web_contents_observer,
    const ShortcutInfo& shortcut_info,
    const SkBitmap& primary_icon,
    WebApkInstallResult result,
    bool relax_updates,
    const std::string& webapk_package_name) {
  installs_.erase(shortcut_info.manifest_url);

  if (result == WebApkInstallResult::SUCCESS) {
    ShowInstallNotification(shortcut_info, primary_icon, webapk_package_name);
    return;
  }

  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedJavaLocalRef<jstring> java_manifest_url =
      base::android::ConvertUTF8ToJavaString(env,
                                             shortcut_info.manifest_url.spec());
  Java_WebApkInstallService_cancelNotification(env, java_manifest_url);

  content::WebContents* web_contents = web_contents_observer->web_contents();
  if (!web_contents)
    return;

  // If the install didn't definitely fail, we don't add a shortcut. This
  // could happen if Play was busy with another install and this one is still
  // queued (and hence might succeed in the future).
  ShortcutHelper::AddToLauncherWithSkBitmap(web_contents, shortcut_info,
                                            primary_icon);
}

// static
void WebApkInstallService::ShowInstallNotification(
    const ShortcutInfo& shortcut_info,
    const SkBitmap& primary_icon,
    const std::string& webapk_package_name) {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedJavaLocalRef<jstring> java_manifest_url =
      base::android::ConvertUTF8ToJavaString(env,
                                             shortcut_info.manifest_url.spec());
  base::android::ScopedJavaLocalRef<jstring> java_title =
      base::android::ConvertUTF16ToJavaString(env, shortcut_info.short_name);
  base::android::ScopedJavaLocalRef<jstring> java_url =
      base::android::ConvertUTF8ToJavaString(env, shortcut_info.url.spec());
  base::android::ScopedJavaLocalRef<jobject> java_primary_icon =
      gfx::ConvertToJavaBitmap(&primary_icon);

  if (webapk_package_name.empty()) {
    Java_WebApkInstallService_showInstallStartNotification(
        env, java_manifest_url, java_title, java_url, java_primary_icon);
    return;
  }

  base::android::ScopedJavaLocalRef<jstring> java_webapk_package =
      base::android::ConvertUTF8ToJavaString(env, webapk_package_name);
  Java_WebApkInstallService_showInstalledNotification(
      env, java_webapk_package, java_manifest_url, java_title, java_url,
      java_primary_icon);
}
