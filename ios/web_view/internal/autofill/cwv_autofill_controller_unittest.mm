// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web_view/internal/autofill/cwv_autofill_controller_internal.h"

#import <Foundation/Foundation.h>
#include <memory>

#import "base/strings/sys_string_conversions.h"
#include "components/autofill/core/browser/autofill_manager.h"
#include "components/autofill/core/common/form_data.h"
#include "components/autofill/core/common/form_field_data.h"
#include "components/autofill/ios/browser/autofill_driver_ios.h"
#import "components/autofill/ios/browser/form_suggestion.h"
#import "ios/testing/wait_util.h"
#import "ios/web/public/test/web_test_with_web_state.h"
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
static NSString* kTestFormHTML =
    [NSString stringWithFormat:
                  @"<html>"
                   "<form name='%@'>"
                   "<input type='text' name='%@' value='%@'/>"
                   "</form>"
                   "</html>",
                  kTestFormName,
                  kTestFieldName,
                  kTestFieldValue];

class CWVAutofillControllerTest : public web::WebTestWithWebState {
 protected:
  void SetUp() override {
    browser_state_.reset(new ios_web_view::WebViewBrowserState(false));

    web::WebTestWithWebState::SetUp();

    l10n_util::OverrideLocaleWithCocoaLocale();

    _autofillController =
        [[CWVAutofillController alloc] initWithWebState:web_state()];
  }

  void TearDown() override {
    web::WebTestWithWebState::TearDown();

    _autofillController = nil;

    browser_state_.reset();
  }

  web::BrowserState* GetBrowserState() { return browser_state_.get(); }

  CWVAutofillController* _autofillController;
  std::unique_ptr<ios_web_view::WebViewBrowserState> browser_state_;
};

// Tests CWVAutofillController returns suggestions.
TEST_F(CWVAutofillControllerTest, FetchSuggestions) {
  autofill::FormData form;
  form.name = base::SysNSStringToUTF16(kTestFormName);

  autofill::FormFieldData field;
  field.label = base::SysNSStringToUTF16(@"fieldLabel");
  field.name = base::SysNSStringToUTF16(kTestFieldName);
  field.value = base::SysNSStringToUTF16(kTestFieldValue);
  field.form_control_type = "text";
  form.fields.push_back(field);

  autofill::AutofillManager* autofillManager =
      autofill::AutofillDriverIOS::FromWebState(web_state())
          ->autofill_manager();
  autofillManager->OnWillSubmitForm(form, base::TimeTicks::Now());
  autofillManager->OnFormSubmitted(form);

  LoadHtml(kTestFormHTML);

  __block BOOL suggestionsFetched = NO;
  id completionHandler = ^(NSArray<CWVAutofillSuggestion*>* suggestions) {
    EXPECT_EQ(1U, suggestions.count);
    CWVAutofillSuggestion* suggestion = suggestions.firstObject;
    EXPECT_NSEQ(kTestFormName, suggestion.formName);
    EXPECT_NSEQ(kTestFieldName, suggestion.fieldName);
    EXPECT_NSEQ(kTestFieldValue, suggestion.value);
    suggestionsFetched = YES;
  };
  [_autofillController fetchSuggestionsForFormWithName:kTestFormName
                                             fieldName:kTestFieldName
                                     completionHandler:completionHandler];

  bool success = testing::WaitUntilConditionOrTimeout(1.0, ^bool {
    WaitForBackgroundTasks();
    return suggestionsFetched;
  });
  EXPECT_TRUE(success);
}

// Tests CWVAutofillController properly fills and clears the form.
TEST_F(CWVAutofillControllerTest, FillAndClearForm) {
  LoadHtml(kTestFormHTML);

  NSString* newFieldValue = @"NewFieldValue";
  FormSuggestion* formSuggestion =
      [FormSuggestion suggestionWithValue:newFieldValue
                       displayDescription:@""
                                     icon:@""
                               identifier:0];
  CWVAutofillSuggestion* autofillSuggestion =
      [[CWVAutofillSuggestion alloc] initWithFormSuggestion:formSuggestion
                                                   formName:kTestFormName
                                                  fieldName:kTestFieldName];

  NSString* selectField =
      [NSString stringWithFormat:@"document.forms[0].%@", kTestFieldName];
  ExecuteJavaScript([selectField stringByAppendingString:@".focus();"]);

  NSString* getValue = [selectField stringByAppendingString:@".value;"];

  __block BOOL formFilled = NO;
  __block BOOL formCleared = NO;
  id formClearedBlock = ^{
    formCleared = YES;
    NSString* value = ExecuteJavaScript(getValue);
    EXPECT_NSEQ(@"", value);
  };
  id formFilledBlock = ^{
    formFilled = YES;
    NSString* value = ExecuteJavaScript(getValue);
    EXPECT_NSEQ(newFieldValue, value);

    [_autofillController clearFormWithName:kTestFormName
                         completionHandler:formClearedBlock];
  };
  [_autofillController fillSuggestion:autofillSuggestion
                    completionHandler:formFilledBlock];

  bool success = testing::WaitUntilConditionOrTimeout(1.0, ^bool {
    return formFilled && formCleared;
  });
  EXPECT_TRUE(success);
}

// Tests CWVAutofillController delegate callbacks are properly invoked.
TEST_F(CWVAutofillControllerTest, DelegateCallbacks) {
  LoadHtml(kTestFormHTML);

  // Create autorelease pool here to ensure delegate is destroyed before
  // tearDown method is called.
  @autoreleasepool {
    id delegate = OCMProtocolMock(@protocol(CWVAutofillControllerDelegate));
    _autofillController.delegate = delegate;

    NSString* selectField =
        [NSString stringWithFormat:@"document.forms[0].%@", kTestFieldName];

    [[delegate expect] autofillController:_autofillController
                  didFocusOnFieldWithName:kTestFieldName
                                 formName:kTestFormName
                                    value:kTestFieldValue];
    ExecuteJavaScript([selectField stringByAppendingString:@".focus();"]);

    [[delegate expect] autofillController:_autofillController
                   didBlurOnFieldWithName:kTestFieldName
                                 formName:kTestFormName
                                    value:kTestFieldValue];
    ExecuteJavaScript([selectField stringByAppendingString:@".blur();"]);

    NSString* newText = @"newText";
    NSString* newValue = [kTestFieldValue stringByAppendingString:newText];
    [[delegate expect] autofillController:_autofillController
                  didInputInFieldWithName:kTestFieldName
                                 formName:kTestFormName
                                    value:newValue];
    ExecuteJavaScript(@"event = document.createEvent('TextEvent');");
    NSString* initTextEvent = [NSString
        stringWithFormat:
            @"event.initTextEvent('textInput', true, true, window, '%@');",
            newText];
    ExecuteJavaScript(initTextEvent);
    ExecuteJavaScript(
        [selectField stringByAppendingString:@".dispatchEvent(event);"]);

    [delegate verify];
  }
}

}  // namespace ios_web_view
