// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBAGENTS_HTML_FORM_ELEMENT_H_
#define WEBAGENTS_HTML_FORM_ELEMENT_H_

#include "third_party/WebKit/webagents/html_element.h"
#include "third_party/WebKit/webagents/webagents_export.h"

namespace blink {
class HTMLFormElement;
}

namespace webagents {

// https://html.spec.whatwg.org/#the-form-element
class WEBAGENTS_EXPORT HTMLFormElement : public HTMLElement {
 public:
 private:
  friend class EventTarget;
  explicit HTMLFormElement(blink::HTMLFormElement&);
  blink::HTMLFormElement& GetHTMLFormElement() const;
};

}  // namespace webagents

#endif  // WEBAGENTS_HTML_FORM_ELEMENT_H_
