// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/html/HTMLIFrameElementDelegateStickyUserActivation.h"

#include "core/html/HTMLIFrameElement.h"
#include "platform/wtf/text/StringBuilder.h"

namespace blink {

namespace {

const char* const kSupportedActivationDelegationTokens[] = {"media"};

bool IsTokenSupported(const AtomicString& token) {
  for (const char* supported_token : kSupportedActivationDelegationTokens) {
    if (token == supported_token)
      return true;
  }
  return false;
}

}  // namespace

HTMLIFrameElementDelegateStickyUserActivation::
    HTMLIFrameElementDelegateStickyUserActivation(HTMLIFrameElement* element)
    : DOMTokenList(*element, HTMLNames::delegatestickyuseractivationAttr) {}

ActivationDelegationFlags
HTMLIFrameElementDelegateStickyUserActivation::GetFlags(
    String& invalid_tokens_error_message) {
  ActivationDelegationFlags flags = kActivationDelegationNone;
  StringBuilder token_errors;
  int number_of_token_errors = 0;
  unsigned length = this->length();

  for (unsigned index = 0; index < length; index++) {
    const AtomicString& token = this->item(index);
    if (EqualIgnoringASCIICase(token, "media")) {
      flags |= kActivationDelegationMedia;
    } else {
      token_errors.Append(token_errors.IsEmpty() ? "'" : ", '");
      token_errors.Append(token);
      token_errors.Append("'");
      number_of_token_errors++;
    }
  }

  if (number_of_token_errors) {
    token_errors.Append(number_of_token_errors > 1
                            ? " are invalid activation delegation values."
                            : " is an invalid activation delegation value.");
    invalid_tokens_error_message = token_errors.ToString();
  }

  return flags;
}

bool HTMLIFrameElementDelegateStickyUserActivation::ValidateTokenValue(
    const AtomicString& token_value,
    ExceptionState&) const {
  return IsTokenSupported(token_value);
}

}  // namespace blink
