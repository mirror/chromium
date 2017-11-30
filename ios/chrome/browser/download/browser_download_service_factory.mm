// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/download/browser_download_service_factory.h"

#include "base/memory/singleton.h"
#include "components/keyed_service/ios/browser_state_dependency_manager.h"
#include "ios/chrome/browser/download/browser_download_service.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// static
BrowserDownloadService* BrowserDownloadServiceFactory::GetForBrowserState(
    web::BrowserState* browser_state) {
  return static_cast<BrowserDownloadService*>(
      GetInstance()->GetServiceForBrowserState(browser_state, true));
}

// static
BrowserDownloadServiceFactory* BrowserDownloadServiceFactory::GetInstance() {
  return base::Singleton<BrowserDownloadServiceFactory>::get();
}

BrowserDownloadServiceFactory::BrowserDownloadServiceFactory()
    : BrowserStateKeyedServiceFactory(
          "BrowserDownloadService",
          BrowserStateDependencyManager::GetInstance()) {}

BrowserDownloadServiceFactory::~BrowserDownloadServiceFactory() = default;

std::unique_ptr<KeyedService>
BrowserDownloadServiceFactory::BuildServiceInstanceFor(
    web::BrowserState* browser_state) const {
  return std::make_unique<BrowserDownloadService>(browser_state);
}
