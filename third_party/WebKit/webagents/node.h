// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBAGENTS_NODE_H_
#define WEBAGENTS_NODE_H_

#include "third_party/WebKit/webagents/event_target.h"
#include "third_party/WebKit/webagents/webagents_export.h"

namespace blink {
class Node;
}

namespace webagents {

class Element;
class HTMLHeadElement;
class HTMLInputElement;
class HTMLMetaElement;
class Text;

// https://dom.spec.whatwg.org/#interface-node
class WEBAGENTS_EXPORT Node : public EventTarget {
 public:
  // readonly attribute Node? parentNode;
  Node parentNode() const;
  // readonly attribute Node? firstChild;
  Node firstChild() const;
  // readonly attribute Node? nextSibling;
  Node nextSibling() const;

  // Not part of DOM.
  bool IsElementNode() const;
  bool IsHTMLMetaElement() const;
  bool IsTextNode() const;

  Element ToElement() const;
  HTMLHeadElement ToHTMLHeadElement() const;
  HTMLInputElement ToHTMLInputElement() const;
  HTMLMetaElement ToHTMLMetaElement() const;
  Text ToText() const;

 protected:
  friend class ChildrenIterable;
  friend class WithDescendantsIterable;
  friend class WithDescendantsIterator;
  explicit Node(blink::Node* node);
  blink::Node* GetNode() const;
};

}  // namespace webagents

#endif  // WEBAGENTS_NODE_H_
