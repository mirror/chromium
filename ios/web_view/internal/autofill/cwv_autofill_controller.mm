// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web_view/internal/autofill/cwv_autofill_controller_internal.h"

#include <memory>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/mac/foundation_util.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "components/autofill/core/browser/autofill_manager.h"
#include "components/autofill/core/browser/keyboard_accessory_metrics_logger.h"
#include "components/autofill/core/common/autofill_pref_names.h"
#include "components/autofill/core/common/autofill_util.h"
#include "components/autofill/core/common/form_data.h"
#include "components/autofill/core/common/form_field_data.h"
#include "components/autofill/ios/browser/autofill_driver_ios.h"
#include "components/autofill/ios/browser/autofill_driver_ios_bridge.h"
#import "components/autofill/ios/browser/js_autofill_manager.h"
#import "ios/web/public/url_scheme_util.h"
#import "ios/web/public/web_state/js/crw_js_injection_receiver.h"
#import "ios/web/public/web_state/web_state_observer_bridge.h"
#include "ios/web_view/internal/app/application_context.h"
#include "ios/web_view/internal/autofill/web_view_autofill_client.h"
#include "ios/web_view/internal/web_view_browser_state.h"
#import "net/base/mac/url_conversions.h"

namespace {
using FormDataVector = std::vector<autofill::FormData>;
typedef void (^FetchFormsCompletionBlock)(BOOL success,
                                          const FormDataVector& formDataVector);
}  // namespace

@interface CWVAutofillController ()<AutofillClientIOSBridge,
                                    AutofillDriverIOSBridge,
                                    CRWWebStateObserver>
@end

@implementation CWVAutofillController {
  // Bridge to observe the web state from Objective-C.
  std::unique_ptr<web::WebStateObserverBridge> _webStateObserverBridge;

  autofill::AutofillManager* _autofillManager;  // weak

  // Manager for Autofill JavaScripts.
  JsAutofillManager* _jsAutofillManager;

  ios_web_view::WebViewBrowserState* _browserState;
}

@synthesize webState = _webState;

- (void)setWebState:(web::WebState*)webState {
  _webState = webState;

  _browserState = ios_web_view::WebViewBrowserState::FromBrowserState(
      webState->GetBrowserState());

  _webStateObserverBridge.reset(
      new web::WebStateObserverBridge(webState, self));

  ios_web_view::WebViewAutofillClient* autofillClient =
      new ios_web_view::WebViewAutofillClient(webState, self);
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
}

- (void)hideAutofillPopup {
}

- (void)onFormDataFilled:(uint16_t)query_id
                  result:(const autofill::FormData&)result {
}

- (void)sendAutofillTypePredictionsToRenderer:
    (const std::vector<autofill::FormStructure*>&)forms {
}

#pragma mark - CRWWebStateObserver

- (void)webState:(web::WebState*)webState didLoadPageWithSuccess:(BOOL)success {
  [_jsAutofillManager inject];
}

- (void)webState:(web::WebState*)webState
    didRegisterFormActivityWithFormNamed:(const std::string&)formName
                               fieldName:(const std::string&)fieldName
                                    type:(const std::string&)type
                                   value:(const std::string&)value
                            inputMissing:(BOOL)inputMissing {
  web::URLVerificationTrustLevel trustLevel;
  const GURL pageURL(webState->GetCurrentURL(&trustLevel));
  //  if (inputMissing || trustLevel !=
  //  web::URLVerificationTrustLevel::kAbsolute ||
  //      !web::UrlHasWebScheme(pageURL) || !webState->ContentIsHTML()) {
  //    return;
  //  }

  if ((type == "blur" || type == "change")) {
    return;
  }

  __weak CWVAutofillController* weakSelf = self;
  std::string fieldNameCopy = fieldName;
  std::string typeCopy = type;
  std::string valueCopy = value;
  [self fetchFormsWithFormName:formName
                         pageURL:pageURL
      minimumRequiredFieldsCount:1
                 completionBlock:^(BOOL success,
                                   const FormDataVector& formDataVector) {
                   if (!success) {
                     return;
                   }
                   autofill::FormData formData = formDataVector[0];
                   [weakSelf fetchSuggestionsForFormData:formData
                                               fieldName:fieldNameCopy
                                                    type:typeCopy
                                                   value:valueCopy];
                 }];
}

- (void)webState:(web::WebState*)webState
    didSubmitDocumentWithFormNamed:(const std::string&)formName
                     userInitiated:(BOOL)userInitiated {
  bool autofillEnabled =
      _browserState->GetPrefs()->GetBoolean(autofill::prefs::kAutofillEnabled);
  if (!autofillEnabled) {
    return;
  }

  web::URLVerificationTrustLevel trustLevel;
  const GURL pageURL(webState->GetCurrentURL(&trustLevel));
  //  if (trustLevel != web::URLVerificationTrustLevel::kAbsolute) {
  //    DLOG(WARNING) << "Form submit not handled on untrusted page";
  //    return;
  //  }

  __weak CWVAutofillController* weakSelf = self;
  id completionBlock = ^(BOOL success, const FormDataVector& formDataVector) {
    CWVAutofillController* strongSelf = weakSelf;
    if (!success || !strongSelf) {
      return;
    }
    autofill::FormData formData = formDataVector[0];
    // iOS doesn't get a separate "will submit form" notification so call
    // OnWillSubmitForm() here.
    strongSelf->_autofillManager->OnWillSubmitForm(formData,
                                                   base::TimeTicks::Now());
    strongSelf->_autofillManager->OnFormSubmitted(formData);
    autofill::KeyboardAccessoryMetricsLogger::OnFormSubmitted();
  };

  [self fetchFormsWithFormName:formName
                         pageURL:pageURL
      minimumRequiredFieldsCount:1
                 completionBlock:completionBlock];
}

- (void)webStateDestroyed:(web::WebState*)webState {
  _webStateObserverBridge.reset();
}

#pragma mark - Private Methods

- (void)fetchFormsWithFormName:(const std::string&)formName
                       pageURL:(const GURL&)pageURL
    minimumRequiredFieldsCount:(NSUInteger)minimumRequiredFieldsCount
               completionBlock:(FetchFormsCompletionBlock)completionBlock {
  base::string16 formName16 = base::UTF8ToUTF16(formName);
  GURL pageURLCopy = pageURL;

  __weak CWVAutofillController* weakSelf = self;
  id completionHandler = ^void(NSString* json) {
    FormDataVector formDataVector;
    BOOL extractionResult = [weakSelf extractFormDataVector:&formDataVector
                                                       json:json
                                                   formName:formName16
                                                    pageURL:pageURLCopy];
    if (completionBlock) {
      completionBlock(extractionResult, formDataVector);
    }
  };
  [_jsAutofillManager
      fetchFormsWithMinimumRequiredFieldsCount:minimumRequiredFieldsCount
                             completionHandler:completionHandler];
}

- (void)fetchSuggestionsForFormData:(const autofill::FormData&)formData
                          fieldName:(const std::string&)fieldName
                               type:(const std::string&)type
                              value:(const std::string&)typedValue {
  // Find the right form and field.
  autofill::FormFieldData formFieldData;
  const base::string16 fieldName16 = base::UTF8ToUTF16(fieldName);
  for (const auto& currentField : formData.fields) {
    if (currentField.name == fieldName16) {
      formFieldData = currentField;
      break;
    }
  }
  if (!formFieldData.SameFieldAs(autofill::FormFieldData())) {
    // Hack to get suggestions from select input elements.
    if (formFieldData.form_control_type == "select-one") {
      // Any value set will cause the AutofillManager to filter suggestions
      // (only show suggestions that begin the same as the current value) with
      // the effect that one only suggestion would be returned; the value
      // itself.
      formFieldData.value = base::string16();
    }
  }

  _autofillManager->OnQueryFormFieldAutofill(0, formData, formFieldData,
                                             gfx::RectF());
}

- (BOOL)extractFormDataVector:(FormDataVector*)formDataVector
                         json:(NSString*)json
                     formName:(const base::string16&)formName
                      pageURL:(const GURL&)pageURL {
  DCHECK(formDataVector);

  // Convert JSON string to JSON object |dataJson|.
  int errorCode = 0;
  std::string errorMessage;
  std::unique_ptr<base::Value> dataJson(base::JSONReader::ReadAndReturnError(
      base::SysNSStringToUTF8(json), base::JSON_PARSE_RFC, &errorCode,
      &errorMessage));
  if (errorCode) {
    LOG(WARNING) << "JSON parse error in form extraction: "
                 << errorMessage.c_str();
    return NO;
  }

  // Returned data should be a dictionary.
  const base::DictionaryValue* data;
  if (!dataJson->GetAsDictionary(&data))
    return NO;

  // Get the list of forms.
  const base::ListValue* formsList;
  if (!data->GetList("forms", &formsList))
    return NO;

  // Iterate through all the extracted forms and copy the data from JSON into
  // AutofillManager structures.
  for (const auto& formDict : *formsList) {
    // Each form list entry should be a JSON dictionary.
    const base::DictionaryValue* formData;
    if (!formDict.GetAsDictionary(&formData))
      return NO;

    // Form data is copied into a FormData object field-by-field.
    autofill::FormData form;
    if (!formData->GetString("name", &form.name))
      return NO;
    if (!formName.empty() && formName != form.name)
      continue;

    // Origin is mandatory.
    base::string16 origin;
    if (!formData->GetString("origin", &origin))
      return NO;

    // Use GURL object to verify origin of host page URL.
    form.origin = GURL(origin);
    if (form.origin.GetOrigin() != pageURL.GetOrigin()) {
      LOG(WARNING) << "Form extraction aborted due to same origin policy";
      return NO;
    }

    // Action is optional.
    base::string16 action;
    formData->GetString("action", &action);
    form.action = GURL(action);

    // Is form tag is optional.
    bool is_form_tag;
    if (formData->GetBoolean("is_form_tag", &is_form_tag))
      form.is_form_tag = is_form_tag;

    // Field list (mandatory) is extracted.
    const base::ListValue* fieldsList;
    if (!formData->GetList("fields", &fieldsList))
      return NO;
    for (const auto& fieldDict : *fieldsList) {
      const base::DictionaryValue* field;
      autofill::FormFieldData fieldData;
      if (fieldDict.GetAsDictionary(&field) &&
          [self extractFormField:*field asFieldData:&fieldData]) {
        form.fields.push_back(fieldData);
      } else {
        return NO;
      }
    }
    formDataVector->push_back(form);
  }
  return YES;
}

// Extracts a single form field from the JSON dictionary into a FormFieldData
// object.
// Returns NO if the field could not be extracted.
- (BOOL)extractFormField:(const base::DictionaryValue&)field
             asFieldData:(autofill::FormFieldData*)fieldData {
  if (!field.GetString("name", &fieldData->name) ||
      !field.GetString("form_control_type", &fieldData->form_control_type)) {
    return NO;
  }

  // Optional fields.
  field.GetString("label", &fieldData->label);
  field.GetString("value", &fieldData->value);
  field.GetString("autocomplete_attribute", &fieldData->autocomplete_attribute);

  int maxLength;
  if (field.GetInteger("max_length", &maxLength))
    fieldData->max_length = maxLength;

  field.GetBoolean("is_autofilled", &fieldData->is_autofilled);

  // TODO(crbug.com/427614): Extract |is_checked|.
  bool isCheckable = false;
  field.GetBoolean("is_checkable", &isCheckable);
  autofill::SetCheckStatus(fieldData, isCheckable, false);

  field.GetBoolean("is_focusable", &fieldData->is_focusable);
  field.GetBoolean("should_autocomplete", &fieldData->should_autocomplete);

  // ROLE_ATTRIBUTE_OTHER is the default value. The only other value as of this
  // writing is ROLE_ATTRIBUTE_PRESENTATION.
  int role;
  if (field.GetInteger("role", &role) &&
      role == autofill::AutofillField::ROLE_ATTRIBUTE_PRESENTATION) {
    fieldData->role = autofill::AutofillField::ROLE_ATTRIBUTE_PRESENTATION;
  }

  // TODO(crbug.com/427614): Extract |text_direction|.

  // Load option values where present.
  const base::ListValue* optionValues;
  if (field.GetList("option_values", &optionValues)) {
    for (const auto& optionValue : *optionValues) {
      base::string16 value;
      if (optionValue.GetAsString(&value))
        fieldData->option_values.push_back(value);
    }
  }

  // Load option contents where present.
  const base::ListValue* optionContents;
  if (field.GetList("option_contents", &optionContents)) {
    for (const auto& optionContent : *optionContents) {
      base::string16 content;
      if (optionContent.GetAsString(&content))
        fieldData->option_contents.push_back(content);
    }
  }

  if (fieldData->option_values.size() != fieldData->option_contents.size())
    return NO;  // Option values and contents lists should match 1-1.

  return YES;
}

@end
