// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/web/web_controller_provider_factory_impl.h"

#include "base/memory/ptr_util.h"
#import "ios/chrome/browser/web/web_controller_provider_impl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

WebControllerProviderFactoryImpl::WebControllerProviderFactoryImpl() {}

WebControllerProviderFactoryImpl::~WebControllerProviderFactoryImpl() {}

std::unique_ptr<ios::WebControllerProvider>
WebControllerProviderFactoryImpl::CreateWebControllerProvider(
    web::BrowserState* browser_state) {
  return base::MakeUnique<WebControllerProviderImpl>(browser_state);
}
