// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/history/nav_tree_page.h"

#include "chrome/browser/history/nav_tree.h"

NavTreePage::NavTreePage(NavTree* tree, const GURL& url)
    : tree_(tree), url_(url) {}

NavTreePage::~NavTreePage() {
  tree_->NavTreePageDestroyed(this);
}

void NavTreePage::AddNode(NavTreeNode* node) {
  nodes_.insert(node);
}

void NavTreePage::RemoveNode(NavTreeNode* node) {
  nodes_.erase(node);
}
