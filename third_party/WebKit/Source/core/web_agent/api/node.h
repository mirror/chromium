// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEB_AGENT_API_NODE_H_
#define WEB_AGENT_API_NODE_H_

#include "core/web_agent/api/event_target.h"

namespace blink {
class Node;
}

namespace web {
class Node : public EventTarget {
 public:
  virtual ~Node() = default;
  static Node* Create(blink::Node*);

 protected:
  explicit Node(blink::Node* node);
  blink::Node* GetNode() const;
};

}  // namespace web
#endif  // WEB_AGENT_API_NODE_H_
