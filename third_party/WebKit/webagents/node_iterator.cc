// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/WebKit/webagents/node_iterator.h"

namespace webagents {

Node WithDescendantsIterator::Next() {
  // Try child.
  Node node = current_;
  Node next = node.firstChild();
  if (next)
    ++depth_;
  // If no children, then next sibling.
  else
    next = node.nextSibling();
  // If no children, and no more siblings, then find next ancestor with
  // sibling
  while (!next) {
    node = node.parentNode();
    // Stop when we get back up to root.
    if (--depth_ <= 0)
      return Node(nullptr);
    next = node.nextSibling();
  }
  return next;
}

}  // namespace webagents
