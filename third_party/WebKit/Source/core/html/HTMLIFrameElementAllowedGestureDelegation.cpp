// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/html/HTMLIFrameElementAllowedGestureDelegation.h"

#include "core/html/HTMLIFrameElement.h"
#include "platform/wtf/text/StringBuilder.h"

namespace blink {

namespace {

const char* const kSupportedGestureDelegationTokens[] = {"media"};

bool IsTokenSupported(const AtomicString& token) {
  for (const char* supported_token : kSupportedGestureDelegationTokens) {
    if (token == supported_token)
      return true;
  }
  return false;
}

}  // namespace

HTMLIFrameElementAllowedGestureDelegation::
    HTMLIFrameElementAllowedGestureDelegation(HTMLIFrameElement* element)
    : DOMTokenList(*element, HTMLNames::allowedgesturedelegationAttr) {}

GestureDelegationFlags HTMLIFrameElementAllowedGestureDelegation::GetFlags(
    String& invalid_tokens_error_message) {
  GestureDelegationFlags flags = kGestureDelegationNone;
  StringBuilder token_errors;
  int number_of_token_errors = 0;
  unsigned length = this->length();

  for (unsigned index = 0; index < length; index++) {
    const AtomicString& token = this->item(index);
    if (EqualIgnoringASCIICase(token, "media")) {
      flags |= kGestureDelegationMedia;
    } else {
      token_errors.Append(token_errors.IsEmpty() ? "'" : ", '");
      token_errors.Append(token);
      token_errors.Append("'");
      number_of_token_errors++;
    }
  }

  if (number_of_token_errors) {
    token_errors.Append(number_of_token_errors > 1
                            ? " are invalid gesture delegation values."
                            : " is an invalid gesture delegation value.");
    invalid_tokens_error_message = token_errors.ToString();
  }

  return flags;
}

bool HTMLIFrameElementAllowedGestureDelegation::ValidateTokenValue(
    const AtomicString& token_value,
    ExceptionState&) const {
  DLOG(ERROR) << token_value;
  return IsTokenSupported(token_value);
}

}  // namespace blink
