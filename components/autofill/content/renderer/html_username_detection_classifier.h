// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/core/common/password_form.h"
#include "third_party/WebKit/public/web/WebFormControlElement.h"
#include "third_party/WebKit/public/web/WebInputElement.h"

namespace autofill {

// Classifier for getting username field by analyzing HTML attribute values.
// The algorithm compares the values with special words, and looks for full or
// partial match.
// Considered values are, in order of importance, and thus priority in making
// the decision, field label, and fiel name concatenated with field id. Special
// words are determined experimentally.
void HTMLGetUsernameField(
    std::vector<blink::WebFormControlElement> control_elements,
    PasswordForm* password_form,
    blink::WebInputElement& username_element);

}  // namespace autofill
