// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/ios/browser/autofill_util.h"

#include "base/json/json_reader.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "components/autofill/core/browser/autofill_field.h"
#include "components/autofill/core/common/autofill_util.h"
#include "components/autofill/core/common/form_data.h"
#include "components/autofill/core/common/form_field_data.h"
#import "ios/web/public/navigation_item.h"
#import "ios/web/public/navigation_manager.h"
#include "ios/web/public/ssl_status.h"
#include "url/gurl.h"

namespace autofill {

bool IsContextSecureForWebState(web::WebState* web_state) {
  // This implementation differs slightly from other platforms. Other platforms'
  // implementations check for the presence of active mixed content, but because
  // the iOS web view blocks active mixed content without an option to run it,
  // there is no need to check for active mixed content here.
  web::NavigationManager* manager = web_state->GetNavigationManager();
  web::NavigationItem* nav_item = manager->GetLastCommittedItem();
  if (!nav_item)
    return false;

  const web::SSLStatus& ssl = nav_item->GetSSL();
  return nav_item->GetURL().SchemeIsCryptographic() && ssl.certificate &&
         (!net::IsCertStatusError(ssl.cert_status) ||
          net::IsCertStatusMinorError(ssl.cert_status));
}

bool GetExtractedFormsData(NSString* formJSON,
                           BOOL filtered,
                           const base::string16& formName,
                           const GURL& pageURL,
                           std::vector<FormData>* formsData) {
  DCHECK(formsData);
  // Convert JSON string to JSON object |dataJson|.
  int errorCode = 0;
  std::string errorMessage;
  std::unique_ptr<base::Value> dataJson(base::JSONReader::ReadAndReturnError(
      base::SysNSStringToUTF8(formJSON), base::JSON_PARSE_RFC, &errorCode,
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
    if (filtered && formName != form.name)
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

    // Is formless checkout tag is optional.
    bool is_formless_checkout;
    if (formData->GetBoolean("is_formless_checkout", &is_formless_checkout))
      form.is_formless_checkout = is_formless_checkout;

    // Field list (mandatory) is extracted.
    const base::ListValue* fieldsList;
    if (!formData->GetList("fields", &fieldsList))
      return NO;
    for (const auto& fieldDict : *fieldsList) {
      const base::DictionaryValue* field;
      autofill::FormFieldData fieldData;
      if (fieldDict.GetAsDictionary(&field) &&
          ExtractFormField(*field, &fieldData)) {
        form.fields.push_back(fieldData);
      } else {
        return NO;
      }
    }
    formsData->push_back(form);
  }
  return YES;
}

bool ExtractFormField(const base::DictionaryValue& field,
                      autofill::FormFieldData* fieldData) {
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

}  // namespace autofill
