// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/WebKit/webagents/html_meta_element.h"

#include "core/html/HTMLMetaElement.h"
#include "public/platform/WebString.h"

namespace webagents {

HTMLMetaElement::HTMLMetaElement(blink::HTMLMetaElement& element)
    : HTMLElement(element) {}

blink::WebString HTMLMetaElement::name() const {
  return ConstUnwrap<blink::HTMLMetaElement>().GetName();
}
blink::WebString HTMLMetaElement::content() const {
  return ConstUnwrap<blink::HTMLMetaElement>().Content();
}

}  // namespace webagents
