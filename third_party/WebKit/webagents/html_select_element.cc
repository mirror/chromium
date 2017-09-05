// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/WebKit/webagents/html_select_element.h"

#include "core/html/HTMLSelectElement.h"

namespace webagents {

HTMLSelectElement::HTMLSelectElement(blink::HTMLSelectElement& element)
    : HTMLElement(element) {}

blink::HTMLSelectElement& HTMLSelectElement::GetHTMLSelectElement() const {
  return toHTMLSelectElement(GetNode());
}

}  // namespace webagents
