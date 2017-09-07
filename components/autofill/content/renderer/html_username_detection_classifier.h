// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/core/common/password_form.h"
#include "third_party/WebKit/public/web/WebFormControlElement.h"
#include "third_party/WebKit/public/web/WebInputElement.h"

namespace autofill {

// Classifier for getting username field by analyzing HTML attribute values.
// The algorithm compares the values with words that are likely to point to
// username field (ex. "username", "loginid" etc.) and looks for full or
// partial match.
// Considered values are, in order of importance (i.e. priority in making the
// decision): field label, and field name concatenated with field id.
// When the first match is found, the currently analyzed field is saved in
// |username_element|, and the algorithm ends. By searching for words in order
// of their probability to be username words, it is sure that the first match
// will also be the best one.
void HTMLGetUsernameField(
    std::vector<blink::WebFormControlElement> control_elements,
    PasswordForm* password_form,
    blink::WebInputElement& username_element);

}  // namespace autofill
