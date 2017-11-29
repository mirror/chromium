// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/webapk/webapk_install_space_manager.h"

#include "jni/WebApkInstallSpaceManager_jni.h"

WebApkInstallSpaceManager::WebApkInstallSpaceManager()
    : space_status_(UNDERDETERMINED) {}

WebApkInstallSpaceManager::~WebApkInstallSpaceManager() {}

bool WebApkInstallSpaceManager::EnoughSpaceToInstall() {
  return space_status_ != NOT_ENOUGH_SPACE;
}

void WebApkInstallSpaceManager::InstallAndFreeCacheIfNecessary() {
  if (space_status_ != ENOUGH_SPACE_AFTER_FREE_UP_CACHE) {
    install_callback_();
    return;
  }
  remover_ = content::BrowserContext::GetBrowsingDataRemover(context_);

  remover_->AddObserver(this);
  int cache_mask = content::BrowsingDataRemover::DATA_TYPE_CACHE_STORAGE |
                   content::BrowsingDataRemover::DATA_TYPE_CACHE;
  remover_->RemoveAndReply(base::Time(), base::Time::Max(), cache_mask,
                           ChromeBrowsingDataRemoverDelegate::ALL_ORIGIN_TYPES,
                           this);
}

void WebApkInstallSpaceManager::OnBrowsingDataRemoverDone() {
  remover_->RemoveObserver(this);
  install_callback_();
}

void WebApkInstallSpaceManager::SetSpaceStatus(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& obj,
    int status) {
  space_status_ = status;
}
