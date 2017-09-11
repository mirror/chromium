// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBAGENTS_NODE_ITERATOR_H_
#define WEBAGENTS_NODE_ITERATOR_H_

#include "base/optional.h"
#include "third_party/WebKit/webagents/node.h"
#include "third_party/WebKit/webagents/webagents_export.h"

namespace webagents {

class ChildrenIterable;
class WithDescendantsIterable;

class WEBAGENTS_EXPORT NodeIterator {
 public:
  static ChildrenIterable ChildrenOf(const Node);
  static WithDescendantsIterable WithDescendants(const Node);
};

class WEBAGENTS_EXPORT ChildrenIterator {
 public:
  ChildrenIterator(base::Optional<Node> current) : current_(current) {}
  Node operator*() const { return *current_; }
  bool operator!=(const ChildrenIterator& rval) const {
    return current_ != rval.current_;
  }
  ChildrenIterator& operator++() {
    current_ = current_->nextSibling();
    return *this;
  }

 private:
  base::Optional<Node> current_;
};

class WEBAGENTS_EXPORT ChildrenIterable {
 public:
  ChildrenIterable(base::Optional<Node> current) : current_(current) {}
  ChildrenIterator begin() { return ChildrenIterator(current_); }
  ChildrenIterator end() { return ChildrenIterator(base::Optional<Node>()); }

 private:
  base::Optional<Node> current_;
};

inline ChildrenIterable NodeIterator::ChildrenOf(const Node parent) {
  return ChildrenIterable(parent.firstChild());
}

class WEBAGENTS_EXPORT WithDescendantsIterator {
 public:
  WithDescendantsIterator(base::Optional<Node> current, int depth)
      : current_(current), depth_(depth) {}
  Node operator*() const { return *current_; }
  bool operator!=(const WithDescendantsIterator& rval) const {
    return current_ != rval.current_;
  }
  WithDescendantsIterator& operator++() {
    current_ = Next();
    return *this;
  }

 private:
  base::Optional<Node> Next();
  base::Optional<Node> current_;
  int depth_;
};

class WEBAGENTS_EXPORT WithDescendantsIterable {
 public:
  WithDescendantsIterable(Node root) : root_(root) {}
  WithDescendantsIterator begin() { return WithDescendantsIterator(root_, 0); }
  WithDescendantsIterator end() {
    return WithDescendantsIterator(base::Optional<Node>(), 0);
  }

 private:
  Node root_;
};

inline WithDescendantsIterable NodeIterator::WithDescendants(const Node root) {
  return WithDescendantsIterable(root);
}

}  // namespace webagents

#endif  // WEBAGENTS_NODE_ITERATOR_H_
