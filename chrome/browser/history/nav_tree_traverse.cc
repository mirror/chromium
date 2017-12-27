// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/history/nav_tree_traverse.h"

#include "chrome/browser/history/nav_tree_node.h"

base::flat_set<NavTreeNode*> NavTreeGetConnectedNodes(NavTreeNode* start) {
  base::flat_set<NavTreeNode*> result;

  base::circular_deque<NavTreeNode*> queue;
  // Avoid many small allocations, this memory is thrown away immediately so
  // wasted space doesn't matter much.
  queue.reserve(32);
  result.insert(start);
  queue.push_back(start);

  while (!queue.empty()) {
    // Everything in the queue has already been added to the result.
    NavTreeNode* cur = queue.front();
    queue.pop_front();

    if (cur->from() && result.count(cur->from()) == 0) {
      result.insert(cur->from());
      queue.push_back(cur->from());
    }

    for (auto& to : cur->to()) {
      if (result.count(to.get()) == 0) {
        result.insert(to.get());
        queue.push_back(to.get());
      }
    }
  }

  return result;
}

/*

NavTreeIterator::NavTreeIterator() = default;
NavTreeIterator::NavTreeIterator(NavTreeIterator&&) noexcept = default;
NavTreeIterator::~NavTreeIterator() = default;

NavTreeIterator& NavTreeIterator::operator=(NavTreeIterator&& other) {
  queue_ = std::move(other.queue_);
  to_index_ = other.to_index_;
  other.to_index_ = kIndexIsFront;
  visited_ = other.visited_;
  return *this;
}

NavTreeIterator& NavTreeIterator::operator++() {
  DCHECK(!queue_.empty());

  while (true) {
    auto front = queue_.front();
    if (to_index_ == kIndexIsFront) {
      if (!front->to().empty())
        to_index_ = 0;  // Advance to first child.
      else
        queue_.pop_front();  // Advance to next node.
This is missing checking that the new current node hasn't been visited yet.
      return *this;
    } else {
      DCHECK(to_index_ < front->to().size());
      ++to_index_;
      if (to_index == front->to().size()) {

  if (to_index_ == front->to().size()

  return *this;
}
*/
