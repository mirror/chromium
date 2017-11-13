// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web_view/internal/autofill/cwv_autofill_controller_internal.h"

#import <Foundation/Foundation.h>
#include <memory>

#include "base/memory/ptr_util.h"
#import "base/strings/sys_string_conversions.h"
#include "components/autofill/ios/browser/autofill_agent.h"
#import "ios/web/public/test/fakes/crw_test_js_injection_receiver.h"
#import "ios/web/public/test/fakes/test_web_state.h"
#include "ios/web/public/test/test_web_thread_bundle.h"
#include "ios/web_view/internal/web_view_browser_state.h"
#import "ios/web_view/public/cwv_autofill_controller_delegate.h"
#import "ios/web_view/public/cwv_autofill_suggestion.h"
#include "testing/gtest/include/gtest/gtest.h"
#import "testing/gtest_mac.h"
#include "testing/platform_test.h"
#include "ui/base/l10n/l10n_util_mac.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface TestAutofillAgent : AutofillAgent

- (instancetype)initWithPrefService:(PrefService*)prefService
                           webState:(web::WebState*)webState
                        suggestions:(NSArray*)suggestions;

@end

@implementation TestAutofillAgent {
  NSArray* _suggestions;
}

- (instancetype)initWithPrefService:(PrefService*)prefService
                           webState:(web::WebState*)webState
                        suggestions:(NSArray*)suggestions {
  self = [super initWithPrefService:prefService webState:webState];
  if (self) {
    _suggestions = suggestions;
  }
  return self;
}

- (void)checkIfSuggestionsAvailableForForm:(NSString*)formName
                                     field:(NSString*)fieldName
                                      type:(NSString*)type
                                typedValue:(NSString*)typedValue
                                  webState:(web::WebState*)webState
                         completionHandler:
                             (SuggestionsAvailableCompletion)completion {
  completion(YES);
}

- (void)retrieveSuggestionsForForm:(NSString*)formName
                             field:(NSString*)fieldName
                              type:(NSString*)type
                        typedValue:(NSString*)typedValue
                          webState:(web::WebState*)webState
                 completionHandler:(SuggestionsReadyCompletion)completion {
  completion(_suggestions, nil);
}

- (void)didSelectSuggestion:(FormSuggestion*)suggestion
                   forField:(NSString*)fieldName
                       form:(NSString*)formName
          completionHandler:(SuggestionHandledCompletion)completion {
  completion();
}

@end

@interface CWVAutofillTestDelegate : NSObject<CWVAutofillControllerDelegate>
@property(nonatomic, readonly) SEL lastSelector;
@property(nonatomic, readonly) NSString* lastFieldName;
@property(nonatomic, readonly) NSString* lastFormName;
@property(nonatomic, readonly) NSString* lastValue;
@end

@implementation CWVAutofillTestDelegate

@synthesize lastSelector = _lastSelector;
@synthesize lastFormName = _lastFormName;
@synthesize lastFieldName = _lastFieldName;
@synthesize lastValue = _lastValue;

#pragma mark - CWVAutofillControllerDelegate

- (void)autofillController:(CWVAutofillController*)autofillController
    didFocusOnFieldWithName:(NSString*)fieldName
                   formName:(NSString*)formName
                      value:(NSString*)value {
  _lastSelector = _cmd;
  _lastFieldName = fieldName;
  _lastFormName = formName;
  _lastValue = value;
}

- (void)autofillController:(CWVAutofillController*)autofillController
    didInputInFieldWithName:(NSString*)fieldName
                   formName:(NSString*)formName
                      value:(NSString*)value {
  _lastSelector = _cmd;
  _lastFieldName = fieldName;
  _lastFormName = formName;
  _lastValue = value;
}

- (void)autofillController:(CWVAutofillController*)autofillController
    didBlurOnFieldWithName:(NSString*)fieldName
                  formName:(NSString*)formName
                     value:(NSString*)value {
  _lastSelector = _cmd;
  _lastFieldName = fieldName;
  _lastFormName = formName;
  _lastValue = value;
}

@end

namespace ios_web_view {

class CWVAutofillControllerTest : public PlatformTest {
 protected:
  void SetUp() override {
    PlatformTest::SetUp();

    l10n_util::OverrideLocaleWithCocoaLocale();

    browserState_.reset(new ios_web_view::WebViewBrowserState(false));
    webState_.reset(new web::TestWebState());

    webState_->SetBrowserState(browserState_.get());
    CRWTestJSInjectionReceiver* injectionReceiver =
        [[CRWTestJSInjectionReceiver alloc] init];
    webState_->SetJSInjectionReceiver(injectionReceiver);
  }

  void TearDown() override {
    PlatformTest::TearDown();

    webState_.reset();
    browserState_.reset();
  }

  std::unique_ptr<web::TestWebState> webState_;
  std::unique_ptr<ios_web_view::WebViewBrowserState> browserState_;

 private:
  web::TestWebThreadBundle web_thread_bundle_;
};

// Tests CWVAutofillController returns suggestions.
TEST_F(CWVAutofillControllerTest, FetchSuggestions) {
  FormSuggestion* formSuggestion =
      [FormSuggestion suggestionWithValue:@"value"
                       displayDescription:@"display"
                                     icon:@"icon"
                               identifier:0];
  TestAutofillAgent* testAutofillAgent =
      [[TestAutofillAgent alloc] initWithPrefService:browserState_->GetPrefs()
                                            webState:webState_.get()
                                         suggestions:@[ formSuggestion ]];
  CWVAutofillController* autofillController =
      [[CWVAutofillController alloc] initWithWebState:webState_.get()
                                        autofillAgent:testAutofillAgent];
  id completionHandler = ^(NSArray<CWVAutofillSuggestion*>* suggestions) {
    EXPECT_EQ(1, (int)suggestions.count);
    CWVAutofillSuggestion* suggestion = suggestions.firstObject;
    EXPECT_NSEQ(formSuggestion.value, suggestion.value);
    EXPECT_NSEQ(formSuggestion.displayDescription,
                suggestion.displayDescription);
  };
  [autofillController fetchSuggestionsForFormWithName:@""
                                            fieldName:@""
                                    completionHandler:completionHandler];
}

// Tests CWVAutofillController delegate callbacks are properly invoked.
TEST_F(CWVAutofillControllerTest, DelegateCallbacks) {
  TestAutofillAgent* testAutofillAgent =
      [[TestAutofillAgent alloc] initWithPrefService:browserState_->GetPrefs()
                                            webState:webState_.get()
                                         suggestions:@[]];
  CWVAutofillController* autofillController =
      [[CWVAutofillController alloc] initWithWebState:webState_.get()
                                        autofillAgent:testAutofillAgent];
  CWVAutofillTestDelegate* delegate = [[CWVAutofillTestDelegate alloc] init];
  autofillController.delegate = delegate;

  NSString* nsFormName = @"TestFormName";
  NSString* nsFieldName = @"TestFieldName";
  NSString* nsValue = @"TestValue";
  std::string formName = base::SysNSStringToUTF8(nsFormName);
  std::string fieldName = base::SysNSStringToUTF8(nsFieldName);
  std::string value = base::SysNSStringToUTF8(nsValue);

  SEL focusSelector =
      @selector(autofillController:didFocusOnFieldWithName:formName:value:);
  webState_->OnFormActivity(formName, fieldName, "focus", value, false);
  EXPECT_NSEQ(nsFormName, delegate.lastFormName);
  EXPECT_NSEQ(nsFieldName, delegate.lastFieldName);
  EXPECT_NSEQ(nsValue, delegate.lastValue);
  EXPECT_EQ(focusSelector, delegate.lastSelector);

  SEL inputSelector =
      @selector(autofillController:didInputInFieldWithName:formName:value:);
  webState_->OnFormActivity(formName, fieldName, "input", value, false);
  EXPECT_NSEQ(nsFormName, delegate.lastFormName);
  EXPECT_NSEQ(nsFieldName, delegate.lastFieldName);
  EXPECT_NSEQ(nsValue, delegate.lastValue);
  EXPECT_EQ(inputSelector, delegate.lastSelector);

  SEL blurSelector =
      @selector(autofillController:didBlurOnFieldWithName:formName:value:);
  webState_->OnFormActivity(formName, fieldName, "blur", value, false);
  EXPECT_NSEQ(nsFormName, delegate.lastFormName);
  EXPECT_NSEQ(nsFieldName, delegate.lastFieldName);
  EXPECT_NSEQ(nsValue, delegate.lastValue);
  EXPECT_EQ(blurSelector, delegate.lastSelector);
}

}  // namespace ios_web_view
