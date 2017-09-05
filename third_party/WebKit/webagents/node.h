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
}

namespace webagents {

class Document;

// https://dom.spec.whatwg.org/#interface-node
class WEBAGENTS_EXPORT Node : public EventTarget {
 public:
  // readonly attribute Document? ownerDocument;
  Document ownerDocument() const;
  // readonly attribute Node? parentNode;
  base::Optional<Node> parentNode() const;
  // readonly attribute Node? firstChild;
  base::Optional<Node> firstChild() const;
  // readonly attribute Node? nextSibling;
  base::Optional<Node> nextSibling() const;

  // Not standard.
  bool NotStandardIsFocused() const;
 protected:
  friend class EventTarget;
  explicit Node(blink::Node& node);
  blink::Node& GetNode() const;
};

}  // namespace webagents

#endif  // WEBAGENTS_NODE_H_
