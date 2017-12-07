// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/history/nav_tree_node.h"

#include "chrome/browser/history/nav_tree_page.h"

NavTreeNode::NavTreeNode(NavTreeNode* parent,
                         scoped_refptr<NavTreePage> page,
                         ui::PageTransition transition_in)
    : parent_(parent),
      page_(std::move(page)) /*, transition_in_(transition_in)*/ {
  page_->AddNode(this);
}

NavTreeNode::~NavTreeNode() {
  page_->RemoveNode(this);
}

void NavTreeNode::ChangePage(NavTreePage* new_page) {
  page_->RemoveNode(this);
  page_ = new_page;
  page_->AddNode(this);
}

void NavTreeNode::AddRevisit(base::Time time) {
  visit_times_.push_back(time);
}
