// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBAGENTS_HTML_OPTION_ELEMENT_H_
#define WEBAGENTS_HTML_OPTION_ELEMENT_H_

#include "base/strings/string16.h"
#include "third_party/WebKit/webagents/html_element.h"
#include "third_party/WebKit/webagents/webagents_export.h"

namespace blink {
class HTMLOptionElement;
}

namespace webagents {

// https://html.spec.whatwg.org/#the-option-element
class WEBAGENTS_EXPORT HTMLOptionElement : public HTMLElement {
 public:
  // [CEReactions] attribute DOMString label;
   base::string16 label() const;
  // [CEReactions] attribute DOMString value;
   base::string16 value() const;
 private:
  friend class EventTarget;
  friend class HTMLInputElement;
  explicit HTMLOptionElement(blink::HTMLOptionElement&);
  blink::HTMLOptionElement& GetHTMLOptionElement() const;
};

}  // namespace webagents

#endif  // WEBAGENTS_HTML_OPTION_ELEMENT_H_
