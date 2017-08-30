// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBAGENTS_HTML_INPUT_ELEMENT_H_
#define WEBAGENTS_HTML_INPUT_ELEMENT_H_

#include <string>
#include "third_party/WebKit/webagents/html_element.h"
#include "third_party/WebKit/webagents/webagents_export.h"

namespace blink {
class HTMLInputElement;
}

namespace webagents {

// https://html.spec.whatwg.org/#the-input-element
class WEBAGENTS_EXPORT HTMLInputElement : public HTMLElement {
 public:
  // [CEReactions] attribute DOMString type;
  std::string type() const;

 private:
  friend class Node;
  explicit HTMLInputElement(blink::HTMLInputElement*);
  blink::HTMLInputElement* GetHTMLInputElement() const;
};

}  // namespace webagents

#endif  // WEBAGENTS_HTML_INPUT_ELEMENT_H_
