// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBAGENTS_HTML_HEAD_ELEMENT_H_
#define WEBAGENTS_HTML_HEAD_ELEMENT_H_

#include <string>
#include "third_party/WebKit/webagents/html_element.h"

namespace blink {
class HTMLHeadElement;
}

namespace webagents {

// https://html.spec.whatwg.org/#htmlheadelement
class HTMLHeadElement : public HTMLElement {
 public:
  explicit HTMLHeadElement(blink::HTMLHeadElement*);
  HTMLHeadElement(Node&);
  virtual ~HTMLHeadElement() = default;

  // [CEReactions] attribute DOMString type;
  std::string type() const;

 protected:
  blink::HTMLHeadElement* GetHTMLHeadElement() const;
};

}  // namespace webagents

#endif  // WEBAGENTS_HTML_HEAD_ELEMENT_H_
