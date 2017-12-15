// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/history/nav_tree_node.h"

#include "chrome/browser/history/nav_tree_page.h"

NavTreeNode::NavTreeNode(NavTreeNode* from,
                         scoped_refptr<NavTreePage> page,
                         int nav_entry_id,
                         ui::PageTransition transition_in)
    : from_(from),
      page_(std::move(page)),
      nav_entry_id_(nav_entry_id),
      transition_in_(transition_in) {
  page_->AddNode(this);
}

NavTreeNode::~NavTreeNode() {
  page_->RemoveNode(this);

  // This will release the pointers to the "to" pages. It's possible that
  // other classes will keep refs to those, so clear the back pointers that
  // would be dangling in that case.
  for (auto& child : to_)
    child->from_ = nullptr;
}

void NavTreeNode::AddTo(scoped_refptr<NavTreeNode> t) {
  to_.emplace_back(std::move(t));
}

void NavTreeNode::ChangePage(NavTreePage* new_page) {
  page_->RemoveNode(this);
  page_ = new_page;
  page_->AddNode(this);
}
