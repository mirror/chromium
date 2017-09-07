// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBAGENTS_NODE_H_
#define WEBAGENTS_NODE_H_

#include "base/optional.h"
#include "third_party/WebKit/webagents/event_target.h"
#include "third_party/WebKit/webagents/webagents_export.h"

namespace blink {
class Node;
class WebString;
}  // namespace blink

namespace webagents {

class Document;
class Element;

// https://dom.spec.whatwg.org/#interface-node
class WEBAGENTS_EXPORT Node : public EventTarget {
 public:
  // readonly attribute Document? ownerDocument;
  Document ownerDocument() const;
  // readonly attribute Node? parentNode;
  base::Optional<Node> parentNode() const;
  // readonly attribute Node? firstChild;
  base::Optional<Node> firstChild() const;
  // readonly attribute Node? previousSibling;
  base::Optional<Node> previousSibling() const;
  // readonly attribute Node? nextSibling;
  base::Optional<Node> nextSibling() const;
  // [CEReactions] attribute DOMString? nodeValue;
  blink::WebString nodeValue() const;
  // Element? querySelector(DOMString selectors);
  base::Optional<Element> querySelector(blink::WebString selectors) const;

  // Not standard.
  bool NotStandardIsFocused() const;

#if INSIDE_BLINK
 protected:
  friend class EventTarget;
  explicit Node(blink::Node& node);
#endif
};

}  // namespace webagents

#endif  // WEBAGENTS_NODE_H_
