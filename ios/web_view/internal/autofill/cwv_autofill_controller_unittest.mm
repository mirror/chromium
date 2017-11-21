// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web_view/internal/autofill/cwv_autofill_controller_internal.h"

#import <Foundation/Foundation.h>
#include <memory>

#import "base/strings/sys_string_conversions.h"
#include "components/autofill/core/browser/autofill_manager.h"
#import "components/autofill/ios/browser/fake_autofill_agent.h"
#import "components/autofill/ios/browser/form_suggestion.h"
#import "ios/testing/wait_util.h"
#import "ios/web/public/test/fakes/crw_test_js_injection_receiver.h"
#import "ios/web/public/test/fakes/test_web_state.h"
#include "ios/web/public/test/test_web_thread_bundle.h"
#include "ios/web/public/web_state/form_activity_params.h"
#import "ios/web_view/internal/autofill/cwv_autofill_suggestion_internal.h"
#include "ios/web_view/internal/web_view_browser_state.h"
#import "ios/web_view/public/cwv_autofill_controller_delegate.h"
#include "testing/gtest/include/gtest/gtest.h"
#import "testing/gtest_mac.h"
#include "testing/platform_test.h"
#import "third_party/ocmock/OCMock/OCMock.h"
#include "ui/base/l10n/l10n_util_mac.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace ios_web_view {

static NSString* kTestFormName = @"FormName";
static NSString* kTestFieldName = @"FieldName";
static NSString* kTestFieldValue = @"FieldValue";

class CWVAutofillControllerTest : public PlatformTest {
 protected:
  void SetUp() override {
    PlatformTest::SetUp();

    l10n_util::OverrideLocaleWithCocoaLocale();

    browser_state_.reset(new ios_web_view::WebViewBrowserState(false));
    web_state_.reset(new web::TestWebState());

    web_state_->SetBrowserState(browser_state_.get());
    CRWTestJSInjectionReceiver* injectionReceiver =
        [[CRWTestJSInjectionReceiver alloc] init];
    web_state_->SetJSInjectionReceiver(injectionReceiver);

    autofill_agent_ = [[FakeAutofillAgent alloc]
        initWithPrefService:browser_state_->GetPrefs()
                   webState:web_state_.get()];

    autofill_controller_ =
        [[CWVAutofillController alloc] initWithWebState:web_state_.get()
                                          autofillAgent:autofill_agent_];
  }

  void TearDown() override {
    PlatformTest::TearDown();

    autofill_controller_ = nil;
    autofill_agent_ = nil;
    web_state_.reset();
    browser_state_.reset();
  }

  std::unique_ptr<web::TestWebState> web_state_;
  std::unique_ptr<ios_web_view::WebViewBrowserState> browser_state_;
  CWVAutofillController* autofill_controller_;
  FakeAutofillAgent* autofill_agent_;

 private:
  web::TestWebThreadBundle web_thread_bundle_;
};

// Tests CWVAutofillController returns and fills suggestions.
TEST_F(CWVAutofillControllerTest, FetchAndFillSuggestions) {
  FormSuggestion* suggestion =
      [FormSuggestion suggestionWithValue:kTestFieldValue
                       displayDescription:nil
                                     icon:nil
                               identifier:0];
  [autofill_agent_ addSuggestion:suggestion
                     forFormName:kTestFormName
                       fieldName:kTestFieldName];

  __block BOOL fetch_completion_was_called = NO;
  __block BOOL fill_completion_was_called = NO;
  id fill_completion = ^{
    fill_completion_was_called = YES;
  };
  id fetch_completion = ^(NSArray<CWVAutofillSuggestion*>* suggestions) {
    EXPECT_EQ(1U, suggestions.count);
    CWVAutofillSuggestion* suggestion = suggestions.firstObject;
    EXPECT_NSEQ(kTestFieldValue, suggestion.value);
    EXPECT_NSEQ(kTestFormName, suggestion.formName);
    EXPECT_NSEQ(kTestFieldName, suggestion.fieldName);

    fetch_completion_was_called = YES;

    [autofill_controller_ fillSuggestion:suggestion
                       completionHandler:fill_completion];
  };
  [autofill_controller_ fetchSuggestionsForFormWithName:kTestFormName
                                              fieldName:kTestFieldName
                                      completionHandler:fetch_completion];

  bool success = testing::WaitUntilConditionOrTimeout(0.5, ^bool {
    return fetch_completion_was_called && fill_completion_was_called;
  });
  EXPECT_TRUE(success);

  EXPECT_NSEQ(suggestion,
              [autofill_agent_ selectedSuggestionForFormName:kTestFormName
                                                   fieldName:kTestFieldName]);
}

// Tests CWVAutofillController clears form.
TEST_F(CWVAutofillControllerTest, ClearForm) {
  __block BOOL clear_form_completion_was_called = NO;
  id clear_form_completion = ^{
    clear_form_completion_was_called = YES;
  };
  [autofill_controller_ clearFormWithName:kTestFormName
                        completionHandler:clear_form_completion];

  bool success = testing::WaitUntilConditionOrTimeout(0.5, ^bool {
    return clear_form_completion_was_called;
  });
  EXPECT_TRUE(success);
}

// Tests CWVAutofillController delegate callbacks are properly invoked.
TEST_F(CWVAutofillControllerTest, DelegateCallbacks) {
  // Create autorelease pool here to ensure delegate is destroyed before
  // tearDown method is called.
  @autoreleasepool {
    id delegate = OCMProtocolMock(@protocol(CWVAutofillControllerDelegate));
    autofill_controller_.delegate = delegate;

    web::FormActivityParams params;
    params.form_name = base::SysNSStringToUTF8(kTestFormName);
    params.field_name = base::SysNSStringToUTF8(kTestFieldName);
    params.value = base::SysNSStringToUTF8(kTestFieldValue);

    [[delegate expect] autofillController:autofill_controller_
                  didFocusOnFieldWithName:kTestFieldName
                                 formName:kTestFormName
                                    value:kTestFieldValue];
    params.type = "focus";
    web_state_->OnFormActivity(params);

    [[delegate expect] autofillController:autofill_controller_
                  didInputInFieldWithName:kTestFieldName
                                 formName:kTestFormName
                                    value:kTestFieldValue];
    params.type = "input";
    web_state_->OnFormActivity(params);

    [[delegate expect] autofillController:autofill_controller_
                   didBlurOnFieldWithName:kTestFieldName
                                 formName:kTestFormName
                                    value:kTestFieldValue];
    params.type = "blur";
    web_state_->OnFormActivity(params);

    [delegate verify];
  }
}

}  // namespace ios_web_view
