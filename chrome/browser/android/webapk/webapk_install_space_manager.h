// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_WEBAPK_WEBAPK_INSTALL_SPACE_MANAGER_H_
#define CHROME_BROWSER_ANDROID_WEBAPK_WEBAPK_INSTALL_SPACE_MANAGER_H_

#include "base/android/jni_android.h"
#include "chrome/browser/browsing_data/chrome_browsing_data_remover_delegate.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browsing_data_remover.h"

// WebApkInstallSpaceManager is the C++ counterpart of
// org.chromium.chrome.browser.webapps's WebApkInstallSpaceManager in Java.
// Checks if there is enough space to install WebApk, free cache if necessary.
class WebApkInstallSpaceManager
    : public content::BrowsingDataRemover::Observer {
 public:
  // Matches order in WebApkInstallSpaceManager.
  enum SpaceStatus {
    ENOUGH_SPACE = 0,
    ENOUGH_SPACE_AFTER_FREE_UP_CACHE = 1,
    NOT_ENOUGH_SPACE = 2,
    UNDERDETERMINED = 3,
  };

  WebApkInstallSpaceManager();

  ~WebApkInstallSpaceManager() override;

  // Fill all necessary fields.
  void Initialize(content::BrowserContext* context,
                  std::function<void()> callback) {
    context_ = context;
    install_callback_ = callback;
  }

  // Return whether we know for sure there is no enough space to install.
  bool EnoughSpaceToInstall();

  // Run the install callback. Free cache if there is no enough space before
  // installing.
  void InstallAndFreeCacheIfNecessary();

  // Set the space status.
  void SetSpaceStatus(JNIEnv* env,
                      const base::android::JavaParamRef<jobject>& obj,
                      int status);

 private:
  void OnBrowsingDataRemoverDone() override;

  int space_status_;

  content::BrowserContext* context_;  // do not own

  std::function<void()> install_callback_;

  content::BrowsingDataRemover* remover_;
};

#endif  // CHROME_BROWSER_ANDROID_WEBAPK_WEBAPK_INSTALL_SPACE_MANAGER_H_
