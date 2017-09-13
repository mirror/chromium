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
}  // namespace blink

namespace webagents {

class CSSStyleDeclaration;
class HTMLCollection;

// https://dom.spec.whatwg.org/#interface-element
// https://w3c.github.io/DOM-Parsing/#extensions-to-the-element-interface
class WEBAGENTS_EXPORT Element : public Node {
 public:
  // readonly attribute DOMString tagName;
  blink::WebString tagName() const;
  // DOMString? getAttribute(DOMString qualifiedName);
  blink::WebString getAttribute(const blink::WebString qualifiedName) const;
  //  boolean hasAttribute(DOMString qualifiedName);
  bool hasAttribute(blink::WebString qualifiedName) const;
  //  HTMLCollection getElementsByTagName(DOMString qualifiedName);
  //  TODO(joelhockey): This is conceptually const.
  HTMLCollection getElementsByTagName(
      const blink::WebString qualifiedName) const;
  // https://w3c.github.io/DOM-Parsing/#extensions-to-the-element-interface
  //  [CEReactions, TreatNullAs=EmptyString] attribute DOMString innerHTML;
  void setInnerHTML(const blink::WebString);

  // Almost standard
  // https://drafts.csswg.org/cssom/#extensions-to-the-window-interface
  // [NewObject] CSSStyleDeclaration getComputedStyle(Element elt, optional
  // CSSOMString? pseudoElt);
  CSSStyleDeclaration getComputedStyle() const;
  // [CEReactions, Reflect] attribute DOMString id;
  blink::WebString GetIdAttribute() const;
  // [CEReactions, Reflect=class] attribute DOMString className;
  blink::WebString GetClassAttribute() const;
  // https://html.spec.whatwg.org/#data-model
  bool IsFocusable() const;

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
