// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_PASSWORDS_FORM_PARSER_H_
#define IOS_CHROME_BROWSER_PASSWORDS_FORM_PARSER_H_

#include <memory>

#include "base/strings/string_util.h"
#include "components/autofill/core/common/form_data.h"
#include "components/autofill/core/common/password_form.h"

namespace passwords {

enum class FormParsingMode { FILLING, SAVING };

class FormParser {
 public:
  FormParser();
  std::unique_ptr<autofill::PasswordForm> Parse(
      const autofill::FormData& form_data,
      FormParsingMode mode);
};

}  // namespace  passwords

#endif  // IOS_CHROME_BROWSER_PASSWORDS_FORM_PARSER_H_
