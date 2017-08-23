// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBAGENTS_HTML_META_ELEMENT_H_
#define WEBAGENTS_HTML_META_ELEMENT_H_

#include <string>
#include "third_party/WebKit/webagents/html_element.h"

namespace blink {
class HTMLMetaElement;
}

namespace webagents {

// https://html.spec.whatwg.org/#the-meta-element
class HTMLMetaElement : public HTMLElement {
 public:
  explicit HTMLMetaElement(blink::HTMLMetaElement*);
  HTMLMetaElement(Node&);
  HTMLMetaElement(const Node&);
  virtual ~HTMLMetaElement() = default;

  // [CEReactions] attribute DOMString name;
  std::string name() const;
  // [CEReactions] attribute DOMString content;
  std::string content() const;

 protected:
  blink::HTMLMetaElement* GetHTMLMetaElement() const;
};

}  // namespace webagents

#endif  // WEBAGENTS_HTML_META_ELEMENT_H_
