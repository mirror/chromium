// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBAGENTS_HTML_LABEL_ELEMENT_H_
#define WEBAGENTS_HTML_LABEL_ELEMENT_H_

#include "base/optional.h"
#include "third_party/WebKit/webagents/html_element.h"
#include "third_party/WebKit/webagents/webagents_export.h"

namespace blink {
class HTMLLabelElement;
}

namespace webagents {

class HTMLElement;

// https://html.spec.whatwg.org/#the-label-element
class WEBAGENTS_EXPORT HTMLLabelElement final : public HTMLElement {
 public:
  // readonly attribute HTMLElement? control;
  base::Optional<HTMLElement> control() const;

#if INSIDE_BLINK
 private:
  friend class EventTarget;
  explicit HTMLLabelElement(blink::HTMLLabelElement&);
#endif
};

}  // namespace webagents

#endif  // WEBAGENTS_HTML_LABEL_ELEMENT_H_
