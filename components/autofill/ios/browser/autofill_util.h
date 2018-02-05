// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_IOS_BROWSER_AUTOFILL_UTIL_H_
#define COMPONENTS_AUTOFILL_IOS_BROWSER_AUTOFILL_UTIL_H_

#include <vector>

#import "ios/web/public/web_state/web_state.h"

class GURL;

namespace autofill {

struct FormData;
struct FormFieldData;

// Checks if current context is secure from an autofill standpoint.
bool IsContextSecureForWebState(web::WebState* web_state);

bool GetExtractedFormsData(NSString* formJSON,
                           BOOL filtered,
                           const base::string16& formName,
                           const GURL& pageURL,
                           std::vector<autofill::FormData>* formsData);
bool ExtractFormField(const base::DictionaryValue& field,
                      FormFieldData* fieldData);

}  // namespace autofill

#endif  // COMPONENTS_AUTOFILL_IOS_BROWSER_AUTOFILL_UTIL_H_
