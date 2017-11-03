// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/web_view/internal/web_view_web_test.h"

#import "ios/web/public/test/fakes/crw_test_js_injection_receiver.h"
#import "ios/web/public/test/fakes/test_web_state.h"
#include "ios/web_view/internal/web_view_browser_state.h"
#include "ui/base/l10n/l10n_util_mac.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace ios_web_view {

WebViewWebTest::WebViewWebTest() = default;

WebViewWebTest::~WebViewWebTest() = default;

void WebViewWebTest::SetUp() {
  PlatformTest::SetUp();

  l10n_util::OverrideLocaleWithCocoaLocale();

  browserState_.reset(new ios_web_view::WebViewBrowserState(false));
  webState_.reset(new web::TestWebState());

  webState_->SetBrowserState(browserState_.get());
  CRWTestJSInjectionReceiver* injectionReceiver =
      [[CRWTestJSInjectionReceiver alloc] init];
  webState_->SetJSInjectionReceiver(injectionReceiver);
}

void WebViewWebTest::TearDown() {
  PlatformTest::TearDown();

  webState_.reset();
  browserState_.reset();
}

}  // namespace ios_web_view
