// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBAGENTS_HTML_ELEMENT_H_
#define WEBAGENTS_HTML_ELEMENT_H_

#include <string>
#include "third_party/WebKit/webagents/element.h"

namespace blink {
class HTMLElement;
}

namespace webagents {

// https://html.spec.whatwg.org/#htmlelement
class HTMLElement : public Element {
 public:
  explicit HTMLElement(blink::HTMLElement*);
  HTMLElement(Node&);
  virtual ~HTMLElement() = default;

  // [CEReactions] attribute DOMString type;
  std::string type() const;

 protected:
  blink::HTMLElement* GetHTMLElement() const;
};

}  // namespace webagents

#endif  // WEBAGENTS_HTML_ELEMENT_H_
