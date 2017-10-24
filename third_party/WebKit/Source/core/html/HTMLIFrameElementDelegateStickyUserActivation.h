// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef HTMLIFrameElementDelegateStickyUserActivation_h
#define HTMLIFrameElementDelegateStickyUserActivation_h

#include "core/dom/DOMTokenList.h"
#include "platform/heap/Handle.h"
#include "public/platform/ActivationDelegationFlags.h"

namespace blink {

class HTMLIFrameElement;

class HTMLIFrameElementDelegateStickyUserActivation final
    : public DOMTokenList {
 public:
  static HTMLIFrameElementDelegateStickyUserActivation* Create(
      HTMLIFrameElement* element) {
    return new HTMLIFrameElementDelegateStickyUserActivation(element);
  }

  ActivationDelegationFlags GetFlags(String& invalid_tokens_error_message);

 private:
  explicit HTMLIFrameElementDelegateStickyUserActivation(HTMLIFrameElement*);
  bool ValidateTokenValue(const AtomicString&, ExceptionState&) const override;
};

}  // namespace blink

#endif
