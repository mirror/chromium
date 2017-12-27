// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_HISTORY_NAV_TREE_TRAVERSE_H_
#define CHROME_BROWSER_HISTORY_NAV_TREE_TRAVERSE_H_

#include <iterator>

#include "base/containers/queue.h"
#include "base/containers/flat_set.h"

class NavTreeNode;

// Returns all NavTreeNodes reachable from the given one.
base::flat_set<NavTreeNode*> NavTreeGetConnectedNodes(NavTreeNode* start);



/*
class NavTreeIterator {
 public:
  using difference_type = ptrdiff_t;
  using value_type = NavTreeNode*;
  using pointer = NavTreeNode**;
  using reference = NavTreeNode*&;
  using iterator_category = std::forward_iterator_tag;

  // Move-only.
  NavTreeIterator();
  NavTreeIterator(NavTreeIterator&& other) noexcept;
  ~NavTreeIterator();

  NavTreeIterator& operator=(NavTreeIterator&& other);

  // Only implement prefix operator++ for advancing the iterator. Since this
  // iterator is very heavyweight we do not want extra copies.
  NavTreeIterator& operator++();

  bool operator==(const ConstViewIterator& other) const;
  bool operator!=(const ConstViewIterator& other) const;
  bool operator<(const ConstViewIterator& other) const;
  bool operator>(const ConstViewIterator& other) const;
  bool operator<=(const ConstViewIterator& other) const;
  bool operator>=(const ConstViewIterator& other) const;

 private:
  // Special value for child_index_, see that variable.
  static constexpr size_t kIndexIsFront = (size_t)(-1);

  base::queue<NavTreeNode> queue_;

  // Indicates the 0-based index into the queue_.front()'s "to" nodes. If this
  // value is kIndexIsFront, then front() is the current node. Otherwise the
  // current node is one of front()'s children.
  size_t to_index_ = kIndexIsFront;

  base::flat_set<NavTreeNode*> visited_;
};
*/

#endif  // CHROME_BROWSER_HISTORY_NAV_TREE_TRAVERSE_H_
