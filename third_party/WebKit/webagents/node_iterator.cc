// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/WebKit/webagents/node_iterator.h"

namespace webagents {

base::Optional<Node> WithDescendantsIterator::Next() {
  // Try child.
  base::Optional<Node> node = current_;
  base::Optional<Node> next = node->firstChild();
  if (next)
    ++depth_;
  // If no children, then next sibling.
  else
    next = node->nextSibling();
  // If no children, and no more siblings, then find next ancestor with
  // sibling
  while (!next) {
    node = node->parentNode();
    // Stop when we get back up to root.
    if (--depth_ <= 0)
      return base::Optional<Node>();
    next = node->nextSibling();
  }
  return next;
}

}  // namespace webagents
