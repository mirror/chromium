// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/WebKit/webagents/element.h"

#include "third_party/WebKit/Source/core/dom/Element.h"

namespace webagents {

Element::Element(blink::Element* element) : Node(element) {}

Element* Element::Create(blink::Element* element) {
  return element ? new Element(element) : nullptr;
}

blink::Element* Element::GetElement() const {
  return blink::ToElement(GetNode());
}

}  // namespace webagents
