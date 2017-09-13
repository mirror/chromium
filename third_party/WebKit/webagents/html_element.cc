// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/WebKit/webagents/html_element.h"

#include "core/html/HTMLElement.h"

namespace webagents {

HTMLElement::HTMLElement(blink::HTMLElement& element) : Element(element) {}

int HTMLElement::offsetWidth() const {
  return const_cast<blink::HTMLElement&>(ConstUnwrap<blink::HTMLElement>())
      .OffsetWidth();
}

}  // namespace webagents
