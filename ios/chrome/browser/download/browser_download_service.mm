// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/download/browser_download_service.h"

#include "base/feature_list.h"
#import "ios/chrome/browser/download/pass_kit_tab_helper.h"
#import "ios/web/public/download/download_controller.h"
#import "ios/web/public/download/download_task.h"
#include "ios/web/public/features.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

BrowserDownloadService::BrowserDownloadService(
    web::BrowserState* browser_state) {
  web::DownloadController::FromBrowserState(browser_state)->SetDelegate(this);
}

BrowserDownloadService::~BrowserDownloadService() = default;

void BrowserDownloadService::OnDownloadCreated(
    web::DownloadController* download_controller,
    web::WebState* web_state,
    std::unique_ptr<web::DownloadTask> task) {
  if (task->GetMimeType() == "application/vnd.apple.pkpass") {
    if (base::FeatureList::IsEnabled(web::features::kNewPassKitDownload)) {
      PassKitTabHelper::FromWebState(web_state)->Download(std::move(task));
    }
  }
}

void BrowserDownloadService::OnDownloadControllerDestroyed(
    web::DownloadController* download_controller) {
  download_controller->SetDelegate(nullptr);
}
