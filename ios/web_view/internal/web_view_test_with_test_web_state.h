// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_VIEW_INTERNAL_WEB_VIEW_TEST_WITH_TEST_WEB_STATE_H_
#define IOS_WEB_VIEW_INTERNAL_WEB_VIEW_TEST_WITH_TEST_WEB_STATE_H_

#include "testing/platform_test.h"

#include <memory>

#include "ios/web/public/test/test_web_thread_bundle.h"

namespace web {
class TestWebState;
}

namespace ios_web_view {

class WebViewBrowserState;

// Test fixture that exposes a TestWebState and a WebViewBrowserState.
class WebViewTestWithTestWebState : public PlatformTest {
 protected:
  WebViewTestWithTestWebState();
  ~WebViewTestWithTestWebState() override;

  // PlatformTest methods.
  void SetUp() override;
  void TearDown() override;

  std::unique_ptr<web::TestWebState> webState_;
  std::unique_ptr<ios_web_view::WebViewBrowserState> browserState_;

 private:
  web::TestWebThreadBundle web_thread_bundle_;
};

}  // namespace ios_web_view

#endif  // IOS_WEB_VIEW_INTERNAL_WEB_VIEW_TEST_WITH_TEST_WEB_STATE_H_
