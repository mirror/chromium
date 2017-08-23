// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBAGENTS_NODE_H_
#define WEBAGENTS_NODE_H_

#include <memory>

#include "third_party/WebKit/webagents/event_target.h"

namespace blink {
class Node;
}

namespace webagents {

// https://dom.spec.whatwg.org/#interface-node
class Node : public EventTarget {
 public:
  // TODO: I want constructor to be protected, but then make_unique
  // doesn't work?
  explicit Node(blink::Node* node);
  virtual ~Node() = default;
  blink::Node* GetNode() const;

  // readonly attribute Node? parentNode;
  std::unique_ptr<Node> parentNode() const;
  // readonly attribute Node? firstChild;
  std::unique_ptr<Node> firstChild() const;
  // readonly attribute Node? nextSibling;
  std::unique_ptr<Node> nextSibling() const;

  // Not part of DOM.
  bool IsElementNode() const;
  bool IsHTMLMetaElement() const;
  bool IsTextNode() const;
};

}  // namespace webagents

#endif  // WEBAGENTS_NODE_H_
