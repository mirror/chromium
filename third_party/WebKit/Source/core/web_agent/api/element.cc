// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/web_agent/api/element.h"

#include "core/dom/Element.h"

namespace web {

Element::Element(blink::Element* element) : Node(element) {}

Element* Element::Create(blink::Element* element) {
  return element ? new Element(element) : nullptr;
}

blink::Element* Element::GetElement() const {
  return blink::ToElement(GetNode());
}

}  // namespace web
