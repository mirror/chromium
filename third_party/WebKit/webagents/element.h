// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBAGENTS_ELEMENT_H_
#define WEBAGENTS_ELEMENT_H_

#include "third_party/WebKit/webagents/node.h"
#include "third_party/WebKit/webagents/webagents_export.h"

namespace blink {
class Element;
struct WebRect;
class WebString;
}

namespace webagents {

// https://dom.spec.whatwg.org/#interface-element
class WEBAGENTS_EXPORT Element : public Node {
 public:
  // readonly attribute DOMString tagName;
  blink::WebString tagName() const;
  // DOMString? getAttribute(DOMString qualifiedName);
  blink::WebString getAttribute(blink::WebString qualifiedName) const;

  // Almost standard
  // [CEReactions, Reflect] attribute DOMString id;
  blink::WebString GetIdAttribute() const;
  // [CEReactions, Reflect=class] attribute DOMString className;
  blink::WebString GetClassAttribute() const;
  //  [CEReactions, TreatNullAs=EmptyString] attribute DOMString innerHTML;
  void setInnerHTML(blink::WebString);

  // Not standard.
  blink::WebRect NotStandardBoundsInViewport() const;
  bool NotStandardIsVisible() const;
  bool NotStandardIsAutofilled() const;

#if INSIDE_BLINK
 protected:
  friend class EventTarget;
  friend class HTMLCollectionIterator;
  explicit Element(blink::Element&);
#endif
};

}  // namespace webagents

#endif  // WEBAGENTS_ELEMENT_H_
