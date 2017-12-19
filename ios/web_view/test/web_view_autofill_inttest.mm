// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <ChromeWebView/ChromeWebView.h>
#import <Foundation/Foundation.h>

#include "base/strings/stringprintf.h"
#import "base/strings/sys_string_conversions.h"
#import "ios/testing/wait_util.h"
#import "ios/web_view/test/web_view_int_test.h"
#import "ios/web_view/test/web_view_test_util.h"
#import "net/base/mac/url_conversions.h"
#include "testing/gtest_mac.h"
#import "third_party/ocmock/OCMock/OCMock.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace ios_web_view {

static NSString* kTestFormName = @"FormName";
static NSString* kTestFieldName = @"FieldName";
static NSString* kTestFieldValue = @"FieldValue";
static NSString* kTestFormHTML =
    [NSString stringWithFormat:
                  @"<form name='%@'>"
                   "<input type='text' name='%@' value='%@'/>"
                   "<input type='submit'/>"
                   "</form>",
                  kTestFormName,
                  kTestFieldName,
                  kTestFieldValue];

// Tests autofill features in CWVWebViews.
typedef ios_web_view::WebViewIntTest WebViewAutofillTest;

// Tests that CWVAutofillControllerDelegate receives did focus callback.
TEST_F(WebViewAutofillTest, TestDelegateDidFocus) {
  CWVAutofillController* autofill_controller = [web_view_ autofillController];
  id delegate = OCMProtocolMock(@protocol(CWVAutofillControllerDelegate));
  autofill_controller.delegate = delegate;

  std::string html = base::SysNSStringToUTF8(kTestFormHTML);
  GURL url = GetUrlForPageWithHtmlBody(html);
  ASSERT_TRUE(test::LoadUrl(web_view_, net::NSURLWithGURL(url)));

  [[delegate expect] autofillController:autofill_controller
                didFocusOnFieldWithName:kTestFieldName
                               formName:kTestFormName
                                  value:kTestFieldValue];
  test::EvaluateJavaScript(web_view_,
                           @"var event = new Event('focus');"
                            "document.forms[0][0].dispatchEvent(event);",
                           nil);
  [delegate verify];
}

// Tests that CWVAutofillControllerDelegate receives did blur callback.
TEST_F(WebViewAutofillTest, TestDelegateDidBlur) {
  CWVAutofillController* autofill_controller = [web_view_ autofillController];
  id delegate = OCMProtocolMock(@protocol(CWVAutofillControllerDelegate));
  autofill_controller.delegate = delegate;

  std::string html = base::SysNSStringToUTF8(kTestFormHTML);
  GURL url = GetUrlForPageWithHtmlBody(html);
  ASSERT_TRUE(test::LoadUrl(web_view_, net::NSURLWithGURL(url)));

  [[delegate expect] autofillController:autofill_controller
                 didBlurOnFieldWithName:kTestFieldName
                               formName:kTestFormName
                                  value:kTestFieldValue];
  test::EvaluateJavaScript(web_view_,
                           @"var event = new Event('blur');"
                            "document.forms[0][0].dispatchEvent(event);",
                           nil);
  [delegate verify];
}

// Tests that CWVAutofillControllerDelegate receives did input callback.
TEST_F(WebViewAutofillTest, TestDelegateDidInput) {
  CWVAutofillController* autofill_controller = [web_view_ autofillController];
  id delegate = OCMProtocolMock(@protocol(CWVAutofillControllerDelegate));
  autofill_controller.delegate = delegate;

  std::string html = base::SysNSStringToUTF8(kTestFormHTML);
  GURL url = GetUrlForPageWithHtmlBody(html);
  ASSERT_TRUE(test::LoadUrl(web_view_, net::NSURLWithGURL(url)));

  [[delegate expect] autofillController:autofill_controller
                didInputInFieldWithName:kTestFieldName
                               formName:kTestFormName
                                  value:kTestFieldValue];
  test::EvaluateJavaScript(web_view_,
                           @"var event = new Event('input', {'bubbles': true});"
                            "document.forms[0][0].dispatchEvent(event);",
                           nil);
  [delegate verify];
}

// Tests that CWVAutofillControllerDelegate receives did submit callback.
TEST_F(WebViewAutofillTest, TestDelegateDidSubmit) {
  std::string html = base::SysNSStringToUTF8(kTestFormHTML);
  GURL url = GetUrlForPageWithHtmlBody(html);
  ASSERT_TRUE(test::LoadUrl(web_view_, net::NSURLWithGURL(url)));

  CWVAutofillController* autofill_controller = [web_view_ autofillController];
  id delegate = OCMProtocolMock(@protocol(CWVAutofillControllerDelegate));
  autofill_controller.delegate = delegate;

  [[delegate expect] autofillController:autofill_controller
                  didSubmitFormWithName:kTestFormName
                          userInitiated:NO
                            isMainFrame:YES];
  test::EvaluateJavaScript(
      web_view_,
      @"var event = new Event('submit', {'bubbles': true});"
       "document.forms[0].dispatchEvent(event);",
      nil);
  [delegate verify];
}

}  // namespace ios_web_view
