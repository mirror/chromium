// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef HTMLIFrameElementAllowedGestureDelegation_h
#define HTMLIFrameElementAllowedGestureDelegation_h

#include "core/dom/DOMTokenList.h"
#include "platform/heap/Handle.h"
#include "public/platform/GestureDelegationFlags.h"

namespace blink {

class HTMLIFrameElement;

class HTMLIFrameElementAllowedGestureDelegation final : public DOMTokenList {
 public:
  static HTMLIFrameElementAllowedGestureDelegation* Create(
      HTMLIFrameElement* element) {
    return new HTMLIFrameElementAllowedGestureDelegation(element);
  }

  GestureDelegationFlags GetFlags(String& invalid_tokens_error_message);

 private:
  explicit HTMLIFrameElementAllowedGestureDelegation(HTMLIFrameElement*);
  bool ValidateTokenValue(const AtomicString&, ExceptionState&) const override;
};

}  // namespace blink

#endif
