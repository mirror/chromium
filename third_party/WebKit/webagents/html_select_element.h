// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBAGENTS_HTML_SELECT_ELEMENT_H_
#define WEBAGENTS_HTML_SELECT_ELEMENT_H_

#include "base/optional.h"
#include "third_party/WebKit/webagents/html_element.h"
#include "third_party/WebKit/webagents/webagents_export.h"

namespace blink {
class HTMLSelectElement;
class WebString;
}  // namespace blink

namespace webagents {

class HTMLFormElement;
class HTMLOptionsCollection;

// https://html.spec.whatwg.org/#the-select-element
class WEBAGENTS_EXPORT HTMLSelectElement final : public HTMLElement {
 public:
  // readonly attribute HTMLFormElement? form;
  base::Optional<HTMLFormElement> form() const;
  // [SameObject] readonly attribute HTMLOptionsCollection options;
  HTMLOptionsCollection options() const;
  // attribute DOMString value;
  blink::WebString value() const;

#if INSIDE_BLINK
 private:
  friend class EventTarget;
  explicit HTMLSelectElement(blink::HTMLSelectElement&);
#endif
};

}  // namespace webagents

#endif  // WEBAGENTS_HTML_SELECT_ELEMENT_H_
