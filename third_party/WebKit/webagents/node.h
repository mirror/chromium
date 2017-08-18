// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBAGENTS_NODE_H_
#define WEBAGENTS_NODE_H_

#include "third_party/WebKit/webagents/event_target.h"

namespace blink {
class Node;
}

namespace webagents {

// https://dom.spec.whatwg.org/#interface-node
class Node : public EventTarget {
 public:
  virtual ~Node() = default;
  static Node* Create(blink::Node*);

  // Not part of DOM.
  bool IsElementNode() const;
  bool IsTextNode() const;

 protected:
  explicit Node(blink::Node* node);
  blink::Node* GetNode() const;
};

}  // namespace webagents

#endif  // WEBAGENTS_NODE_H_
