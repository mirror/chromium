// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/history/nav_tree_page.h"

NavTreePage::NavTreePage(const GURL& url) : url_(url) {}
NavTreePage::~NavTreePage() {}

void NavTreePage::AddNode(NavTreeNode* node) {
  nodes_.insert(node);
}

void NavTreePage::RemoveNode(NavTreeNode* node) {
  nodes_.erase(node);
}
