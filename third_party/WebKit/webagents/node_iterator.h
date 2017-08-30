// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBAGENTS_NODE_ITERATOR_H_
#define WEBAGENTS_NODE_ITERATOR_H_

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
  ChildrenIterator(Node current) : current_(current) {}
  Node operator*() const { return current_; }
  bool operator!=(const ChildrenIterator& rval) const {
    return current_ != rval.current_;
  }
  ChildrenIterator& operator++() {
    current_ = current_.nextSibling();
    return *this;
  }

 private:
  Node current_;
};

class WEBAGENTS_EXPORT ChildrenIterable {
 public:
  ChildrenIterable(Node current) : current_(current) {}
  ChildrenIterator begin() { return ChildrenIterator(current_); }
  ChildrenIterator end() { return ChildrenIterator(Node(nullptr)); }

 private:
  Node current_;
};

inline ChildrenIterable NodeIterator::ChildrenOf(const Node parent) {
  return ChildrenIterable(parent.firstChild());
}

class WEBAGENTS_EXPORT WithDescendantsIterator {
 public:
  WithDescendantsIterator(Node current, int depth)
      : current_(current), depth_(depth) {}
  Node operator*() const { return current_; }
  bool operator!=(const WithDescendantsIterator& rval) const {
    return current_ != rval.current_;
  }
  WithDescendantsIterator& operator++() {
    current_ = Next();
    return *this;
  }

 private:
  Node Next();
  Node current_;
  int depth_;
};

class WEBAGENTS_EXPORT WithDescendantsIterable {
 public:
  WithDescendantsIterable(Node root) : root_(root) {}
  WithDescendantsIterator begin() { return WithDescendantsIterator(root_, 0); }
  WithDescendantsIterator end() {
    return WithDescendantsIterator(Node(nullptr), 0);
  }

 private:
  Node root_;
};

inline WithDescendantsIterable NodeIterator::WithDescendants(const Node root) {
  return WithDescendantsIterable(root);
}

}  // namespace webagents

#endif  // WEBAGENTS_NODE_ITERATOR_H_
