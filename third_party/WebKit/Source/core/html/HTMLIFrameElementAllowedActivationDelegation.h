// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef HTMLIFrameElementAllowedActivationDelegation_h
#define HTMLIFrameElementAllowedActivationDelegation_h

#include "core/dom/DOMTokenList.h"
#include "platform/heap/Handle.h"
#include "public/platform/ActivationDelegationFlags.h"

namespace blink {

class HTMLIFrameElement;

class HTMLIFrameElementAllowedActivationDelegation final : public DOMTokenList {
 public:
  static HTMLIFrameElementAllowedActivationDelegation* Create(
      HTMLIFrameElement* element) {
    return new HTMLIFrameElementAllowedActivationDelegation(element);
  }

  ActivationDelegationFlags GetFlags(String& invalid_tokens_error_message);

 private:
  explicit HTMLIFrameElementAllowedActivationDelegation(HTMLIFrameElement*);
  bool ValidateTokenValue(const AtomicString&, ExceptionState&) const override;
};

}  // namespace blink

#endif
