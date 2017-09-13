// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBAGENTS_HTML_ELEMENT_H_
#define WEBAGENTS_HTML_ELEMENT_H_

#include <string>
#include "third_party/WebKit/webagents/element.h"
#include "third_party/WebKit/webagents/webagents_export.h"

namespace blink {
class HTMLElement;
}

namespace webagents {

// https://html.spec.whatwg.org/#htmlelement
// https://drafts.csswg.org/cssom-view/#extensions-to-the-htmlelement-interface
class WEBAGENTS_EXPORT HTMLElement : public Element {
 public:
  // readonly attribute long offsetWidth;
  int offsetWidth() const;

#if INSIDE_BLINK
 protected:
  friend class EventTarget;
  explicit HTMLElement(blink::HTMLElement&);
#endif
};

}  // namespace webagents

#endif  // WEBAGENTS_HTML_ELEMENT_H_
