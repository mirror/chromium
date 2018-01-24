// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <ChromeWebView/ChromeWebView.h>
#import <Foundation/Foundation.h>

#include "base/strings/stringprintf.h"
#include "base/strings/sys_string_conversions.h"
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

using testing::kWaitForActionTimeout;
using testing::WaitUntilConditionOrTimeout;

namespace ios_web_view {

namespace {
NSString* const kTestFormName = @"FormName";
NSString* const kTestFieldName = @"FieldName";
NSString* const kTestFieldValue = @"FieldValue";
NSString* const kTestFormHtml =
    [NSString stringWithFormat:
                  @"<form name='%@'>"
                   "<input type='text' name='%@' value='%@'/>"
                   "<input type='submit'/>"
                   "</form>",
                  kTestFormName,
                  kTestFieldName,
                  kTestFieldValue];
}  // namespace

// Tests autofill features in CWVWebViews.
class WebViewAutofillTest : public WebViewIntTest {
 protected:
  void SetUp() override {
    WebViewIntTest::SetUp();

    std::string html = base::SysNSStringToUTF8(kTestFormHtml);
    GURL url = GetUrlForPageWithHtmlBody(html);
    ASSERT_TRUE(test::LoadUrl(web_view_, net::NSURLWithGURL(url)));
  }
};

// Tests that CWVAutofillControllerDelegate receives callbacks.
TEST_F(WebViewAutofillTest, TestDelegateCallbacks) {
  CWVAutofillController* autofill_controller = [web_view_ autofillController];
  id delegate = OCMProtocolMock(@protocol(CWVAutofillControllerDelegate));
  autofill_controller.delegate = delegate;

  [[delegate expect] autofillController:autofill_controller
                didFocusOnFieldWithName:kTestFieldName
                               formName:kTestFormName
                                  value:kTestFieldValue];
  test::EvaluateJavaScript(web_view_,
                           @"var event = new Event('focus');"
                            "document.forms[0][0].dispatchEvent(event);",
                           nil);
  [delegate verify];

  [[delegate expect] autofillController:autofill_controller
                 didBlurOnFieldWithName:kTestFieldName
                               formName:kTestFormName
                                  value:kTestFieldValue];
  test::EvaluateJavaScript(web_view_,
                           @"var event = new Event('blur');"
                            "document.forms[0][0].dispatchEvent(event);",
                           nil);
  [delegate verify];

  [[delegate expect] autofillController:autofill_controller
                didInputInFieldWithName:kTestFieldName
                               formName:kTestFormName
                                  value:kTestFieldValue];
  // The 'input' event listener defined in form.js is only called during the
  // bubbling phase.
  test::EvaluateJavaScript(web_view_,
                           @"var event = new Event('input', {'bubbles': true});"
                            "document.forms[0][0].dispatchEvent(event);",
                           nil);
  [delegate verify];

  [[delegate expect] autofillController:autofill_controller
                  didSubmitFormWithName:kTestFormName
                          userInitiated:NO
                            isMainFrame:YES];
  // The 'submit' event listener defined in form.js is only called during the
  // bubbling phase.
  test::EvaluateJavaScript(
      web_view_,
      @"var event = new Event('submit', {'bubbles': true});"
       "document.forms[0].dispatchEvent(event);",
      nil);
  [delegate verify];
}

// Tests that CWVAutofillController can fetch, fill, and clear suggestions.
TEST_F(WebViewAutofillTest, TestSuggestionFetchFillClear) {
  test::EvaluateJavaScript(web_view_, @"document.forms[0][1].click();", nil);

  __block bool suggestions_fetched = false;
  [[web_view_ autofillController]
      fetchSuggestionsForFormWithName:kTestFormName
                            fieldName:kTestFieldName
                    completionHandler:^(
                        NSArray<CWVAutofillSuggestion*>* suggestions) {
                      suggestions_fetched = true;
                    }];
  EXPECT_TRUE(WaitUntilConditionOrTimeout(kWaitForActionTimeout, ^bool {
    WaitForBackgroundTasks();
    return suggestions_fetched;
  }));
}

}  // namespace ios_web_view
