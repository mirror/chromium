// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBAGENTS_HTML_SELECT_ELEMENT_H_
#define WEBAGENTS_HTML_SELECT_ELEMENT_H_

#include "third_party/WebKit/webagents/html_element.h"
#include "third_party/WebKit/webagents/webagents_export.h"

namespace blink {
class HTMLSelectElement;
}

namespace webagents {

// https://html.spec.whatwg.org/#the-select-element
class WEBAGENTS_EXPORT HTMLSelectElement : public HTMLElement {
 public:
 private:
  friend class EventTarget;
  explicit HTMLSelectElement(blink::HTMLSelectElement&);
  blink::HTMLSelectElement& GetHTMLSelectElement() const;
};

}  // namespace webagents

#endif  // WEBAGENTS_HTML_SELECT_ELEMENT_H_
