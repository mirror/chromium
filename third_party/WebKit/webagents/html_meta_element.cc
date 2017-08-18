// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/WebKit/webagents/html_meta_element.h"

#include "third_party/WebKit/Source/core/html/HTMLMetaElement.h"

namespace webagents {

HTMLMetaElement::HTMLMetaElement(blink::HTMLMetaElement* element)
    : HTMLElement(element) {}

HTMLMetaElement* HTMLMetaElement::Create(blink::HTMLMetaElement* element) {
  return element ? new HTMLMetaElement(element) : nullptr;
}

blink::HTMLMetaElement* HTMLMetaElement::GetHTMLMetaElement() const {
  return toHTMLMetaElement(GetNode());
}

std::string HTMLMetaElement::name() const {
  return blink::WebString(GetHTMLMetaElement()->GetName()).Ascii();
}
std::string HTMLMetaElement::content() const {
  return blink::WebString(GetHTMLMetaElement()->Content()).Ascii();
}

}  // namespace webagents
