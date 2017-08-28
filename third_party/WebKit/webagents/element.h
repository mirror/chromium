// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBAGENTS_ELEMENT_H_
#define WEBAGENTS_ELEMENT_H_

#include <string>
#include "third_party/WebKit/webagents/node.h"

namespace blink {
class Element;
}

namespace webagents {

// https://dom.spec.whatwg.org/#interface-element
class Element : public Node {
 public:
  explicit Element(blink::Element*);
  Element(Node&);
  virtual ~Element() = default;

  // readonly attribute DOMString tagName;
  std::string tagName() const;
  // DOMString? getAttribute(DOMString qualifiedName);
  std::string getAttribute(std::string qualifiedName) const;

  // Almost standard
  // [CEReactions, Reflect] attribute DOMString id;
  std::string GetIdAttribute() const;
  // [CEReactions, Reflect=class] attribute DOMString className;
  std::string GetClassAttribute() const;
  //  [CEReactions, TreatNullAs=EmptyString] attribute DOMString innerHTML;
  void setInnerHTML(std::string);

  // Not standard DOM.
  bool IsVisible() const;

 protected:
  blink::Element* GetElement() const;
};

}  // namespace webagents

#endif  // WEBAGENTS_ELEMENT_H_
