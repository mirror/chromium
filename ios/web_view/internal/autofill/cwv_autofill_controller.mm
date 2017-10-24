// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web_view/internal/autofill/cwv_autofill_controller_internal.h"

#include <memory>

#include "base/callback.h"
#include "base/mac/foundation_util.h"
#include "base/strings/sys_string_conversions.h"
#include "components/autofill/core/browser/autofill_manager.h"
#include "components/autofill/core/browser/popup_item_ids.h"
#include "components/autofill/ios/browser/autofill_agent.h"
#import "components/autofill/ios/browser/autofill_client_ios.h"
#include "components/autofill/ios/browser/autofill_driver_ios.h"
#include "components/autofill/ios/browser/autofill_driver_ios_bridge.h"
#import "components/autofill/ios/browser/js_autofill_manager.h"
#include "components/keyed_service/core/service_access_type.h"
#include "components/signin/core/browser/profile_identity_provider.h"
#include "components/signin/core/browser/signin_manager.h"
#import "ios/web/public/web_state/js/crw_js_injection_receiver.h"
#import "ios/web/public/web_state/web_state_observer_bridge.h"
#include "ios/web_view/internal/app/application_context.h"
#import "ios/web_view/internal/autofill/cwv_autofill_form_suggestion_internal.h"
#include "ios/web_view/internal/autofill/web_view_personal_data_manager_factory.h"
#include "ios/web_view/internal/signin/web_view_oauth2_token_service_factory.h"
#include "ios/web_view/internal/signin/web_view_signin_manager_factory.h"
#include "ios/web_view/internal/web_view_browser_state.h"
#include "ios/web_view/internal/webdata_services/web_view_web_data_service_wrapper_factory.h"
#import "ios/web_view/public/cwv_autofill_controller_delegate.h"

@interface CWVAutofillController ()<AutofillClientIOSBridge,
                                    AutofillDriverIOSBridge,
                                    CRWWebStateObserver>
@end

@implementation CWVAutofillController {
  // Bridge to observe the web state from Objective-C.
  std::unique_ptr<web::WebStateObserverBridge> _webStateObserverBridge;

  AutofillAgent* _autofillAgent;

  // weak
  autofill::AutofillManager* _autofillManager;

  ios_web_view::WebViewBrowserState* _browserState;

  // Exposes injected javascript methods related to autofill.
  JsAutofillManager* _jsAutofillManager;
}

@synthesize delegate = _delegate;
@synthesize webState = _webState;

#pragma mark - Public Methods

- (void)clearFormNamed:(NSString*)formName {
  [_jsAutofillManager clearAutofilledFieldsForFormNamed:formName
                                      completionHandler:^{
                                      }];
}

- (void)fetchSuggestionsForFormName:(NSString*)formName
                          fieldName:(NSString*)fieldName
                         typedValue:(NSString*)typedValue
                    completionBlock:
                        (void (^)(NSArray<CWVAutofillFormSuggestion*>*))
                            completionBlock {
  __weak CWVAutofillController* weakSelf = self;
  id completionHandler = ^(BOOL suggestionsAvailable) {
    CWVAutofillController* strongSelf = weakSelf;
    if (!strongSelf) {
      return;
    }
    id completionHandler =
        ^(NSArray* suggestions, id<FormSuggestionProvider> delegate) {
          NSMutableArray* cwvFormSuggestions = [NSMutableArray array];
          for (FormSuggestion* formSuggestion in suggestions) {
            CWVAutofillFormSuggestion* cwvFormSuggestion =
                [[CWVAutofillFormSuggestion alloc]
                    initWithFormSuggestion:formSuggestion
                                  formName:formName
                                 fieldName:fieldName];
            [cwvFormSuggestions addObject:cwvFormSuggestion];
          }
          completionBlock(cwvFormSuggestions);
        };
    [strongSelf->_autofillAgent retrieveSuggestionsForForm:formName
                                                     field:fieldName
                                                      type:nil
                                                typedValue:typedValue
                                                  webState:strongSelf.webState
                                         completionHandler:completionHandler];
  };
  [_autofillAgent checkIfSuggestionsAvailableForForm:formName
                                               field:fieldName
                                                type:nil
                                          typedValue:typedValue
                                            webState:_webState
                                   completionHandler:completionHandler];
}

- (void)fillFormSuggestion:(CWVAutofillFormSuggestion*)formSuggestion
                  formName:(NSString*)formName
                 fieldName:(NSString*)fieldName {
  [_autofillAgent didSelectSuggestion:formSuggestion.formSuggestion
                             forField:fieldName
                                 form:formName
                    completionHandler:^{
                    }];
}

#pragma mark - Internal

- (void)setWebState:(web::WebState*)webState {
  _webState = webState;

  _browserState = ios_web_view::WebViewBrowserState::FromBrowserState(
      webState->GetBrowserState());
  _autofillAgent =
      [[AutofillAgent alloc] initWithPrefService:_browserState->GetPrefs()
                                        webState:webState];

  _webStateObserverBridge.reset(
      new web::WebStateObserverBridge(webState, self));

  std::unique_ptr<IdentityProvider> identityProvider(
      new ProfileIdentityProvider(
          ios_web_view::WebViewSigninManagerFactory::GetForBrowserState(
              _browserState),
          ios_web_view::WebViewOAuth2TokenServiceFactory::GetForBrowserState(
              _browserState),
          base::Closure()));
  autofill::AutofillClientIOS* autofillClient = new autofill::AutofillClientIOS(
      _browserState->GetPrefs(),
      ios_web_view::WebViewPersonalDataManagerFactory::GetForBrowserState(
          _browserState),
      _webState, self, std::move(identityProvider),
      ios_web_view::WebViewWebDataServiceWrapperFactory::
          GetAutofillWebDataForBrowserState(
              _browserState, ServiceAccessType::EXPLICIT_ACCESS));
  autofill::AutofillDriverIOS::CreateForWebStateAndDelegate(
      webState, autofillClient, self,
      ios_web_view::ApplicationContext::GetInstance()->GetApplicationLocale(),
      autofill::AutofillManager::ENABLE_AUTOFILL_DOWNLOAD_MANAGER);
  _autofillManager =
      autofill::AutofillDriverIOS::FromWebState(webState)->autofill_manager();

  _jsAutofillManager = base::mac::ObjCCastStrict<JsAutofillManager>(
      [webState->GetJSInjectionReceiver()
          instanceOfClass:[JsAutofillManager class]]);
}

#pragma mark - AutofillClientIOSBridge | AutofillDriverIOSBridge

- (void)showAutofillPopup:(const std::vector<autofill::Suggestion>&)suggestions
            popupDelegate:
                (const base::WeakPtr<autofill::AutofillPopupDelegate>&)
                    delegate {
  NSMutableArray* formSuggestions = [[NSMutableArray alloc] init];
  for (const auto& suggestion : suggestions) {
    NSString* value = nil;
    NSString* displayDescription = nil;
    if (suggestion.frontend_id >= 0) {
      value = base::SysUTF16ToNSString(suggestion.value);
      displayDescription = base::SysUTF16ToNSString(suggestion.label);
    }

    if (!value)
      continue;

    NSString* icon = base::SysUTF16ToNSString(suggestion.icon);
    NSInteger identifier = suggestion.frontend_id;

    FormSuggestion* formSuggestion =
        [FormSuggestion suggestionWithValue:value
                         displayDescription:displayDescription
                                       icon:icon
                                 identifier:identifier];
    [formSuggestions addObject:formSuggestion];
  }

  [_autofillAgent onSuggestionsReady:formSuggestions popupDelegate:delegate];
  if (delegate) {
    delegate->OnPopupShown();
  }
}

- (void)hideAutofillPopup {
  [_autofillAgent
      onSuggestionsReady:@[]
           popupDelegate:base::WeakPtr<autofill::AutofillPopupDelegate>()];
}

- (void)onFormDataFilled:(uint16_t)query_id
                  result:(const autofill::FormData&)result {
  [_autofillAgent onFormDataFilled:result];
  if (_autofillManager)
    _autofillManager->OnDidFillAutofillFormData(result, base::TimeTicks::Now());
}

- (void)sendAutofillTypePredictionsToRenderer:
    (const std::vector<autofill::FormStructure*>&)forms {
  // Not supported.
}

#pragma mark - CRWWebStateObserver

- (void)webState:(web::WebState*)webState
    didRegisterFormActivityWithFormNamed:(const std::string&)formName
                               fieldName:(const std::string&)fieldName
                                    type:(const std::string&)type
                                   value:(const std::string&)value
                            inputMissing:(BOOL)inputMissing {
  NSString* nsFormName = base::SysUTF8ToNSString(formName);
  NSString* nsFieldName = base::SysUTF8ToNSString(fieldName);
  NSString* nsValue = base::SysUTF8ToNSString(value);

  if (type == "focus") {
    if ([_delegate
            respondsToSelector:@selector
            (autofillController:didFocusOnFormName:fieldName:typedValue:)]) {
      [_delegate autofillController:self
                 didFocusOnFormName:nsFormName
                          fieldName:nsFieldName
                         typedValue:nsValue];
    }
  } else if (type == "input") {
    if ([_delegate
            respondsToSelector:@selector
            (autofillController:didInputInFormName:fieldName:typedValue:)]) {
      [_delegate autofillController:self
                 didInputInFormName:nsFormName
                          fieldName:nsFieldName
                         typedValue:nsValue];
    }
  } else if (type == "blur") {
    if ([_delegate
            respondsToSelector:@selector
            (autofillController:didBlurOnFormName:fieldName:typedValue:)]) {
      [_delegate autofillController:self
                  didBlurOnFormName:nsFormName
                          fieldName:nsFieldName
                         typedValue:nsValue];
    }
  }
}

- (void)webStateDestroyed:(web::WebState*)webState {
  _webStateObserverBridge.reset();
}

@end
