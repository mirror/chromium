// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/web_view/internal/web_view_web_client.h"

#import <UIKit/UIKit.h>

#include <memory>

#import "ios/web/public/test/js_test_util.h"
#include "ios/web/public/test/scoped_testing_web_client.h"
#include "ios/web/public/test/test_web_thread_bundle.h"
#import "ios/web/public/web_view_creation_util.h"
#include "ios/web_view/internal/web_view_browser_state.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gtest_mac.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

class WebViewWebClientTest : public PlatformTest {
 public:
  WebViewWebClientTest() : browser_state_(/*off_the_record=*/false) {}

  ~WebViewWebClientTest() override = default;

  ios_web_view::WebViewBrowserState* browser_state() { return &browser_state_; }

 private:
  web::TestWebThreadBundle web_thread_bundle_;
  ios_web_view::WebViewBrowserState browser_state_;

  DISALLOW_COPY_AND_ASSIGN(WebViewWebClientTest);
};

// Tests that WebViewWebClient provides autofill controller script for
// WKWebView.
TEST_F(WebViewWebClientTest, WKWebViewEarlyPageScriptAutofillController) {
  web::ScopedTestingWebClient web_client(
      std::make_unique<ios_web_view::WebViewWebClient>());
  web_client.Get()->SetProduct();

  // autofill controller script relies on __gCrWeb object presence.
  WKWebView* web_view = web::BuildWKWebView(CGRectZero, browser_state());
  web::ExecuteJavaScript(web_view, @"__gCrWeb = {};");

  NSString* script =
      web_client.Get()->GetEarlyPageScriptForMainFrame(browser_state());
  web::ExecuteJavaScript(web_view, script);
  EXPECT_NSEQ(@"object",
              web::ExecuteJavaScript(web_view, @"typeof __gCrWeb.autofill"));
}

}  // namespace
