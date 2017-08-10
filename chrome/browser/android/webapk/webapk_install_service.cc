// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/webapk/webapk_install_service.h"

#include "base/bind.h"
#include "chrome/browser/android/shortcut_helper.h"
#include "chrome/browser/android/shortcut_info.h"
#include "chrome/browser/android/webapk/webapk_install_service_factory.h"
#include "chrome/browser/android/webapk/webapk_installer.h"

namespace {
// Called after the install of a WebAPK is either completed or failed.
void OnWebApkInstallFinished(content::WebContents* web_contents,
                             const ShortcutInfo& info,
                             const SkBitmap& icon_bitmap,
                             WebApkInstallResult result,
                             bool relax_updates,
                             const std::string& webapk_package_name) {
  if (result != WebApkInstallResult::FAILURE)
    return;

  ShortcutHelper::AddToLauncherWithSkBitmap(web_contents, info, icon_bitmap);
}

}  // anonymous namespace

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
  FinishCallback callback = base::Bind(OnWebApkInstallFinished, web_contents,
                                       shortcut_info, primary_icon);
  installs_.insert(shortcut_info.manifest_url);

  WebApkInstaller::InstallAsync(
      browser_context_, shortcut_info, primary_icon, badge_icon,
      base::Bind(&WebApkInstallService::OnFinishedInstall,
                 weak_ptr_factory_.GetWeakPtr(), shortcut_info.manifest_url,
                 callback));
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
    const GURL& web_manifest_url,
    const FinishCallback& finish_callback,
    WebApkInstallResult result,
    bool relax_updates,
    const std::string& webapk_package_name) {
  finish_callback.Run(result, relax_updates, webapk_package_name);
  installs_.erase(web_manifest_url);
}
