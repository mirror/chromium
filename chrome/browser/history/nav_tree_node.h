// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_HISTORY_NAV_TREE_NODE_H_
#define CHROME_BROWSER_HISTORY_NAV_TREE_NODE_H_

#include <vector>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/time/time.h"
#include "ui/base/page_transition_types.h"
#include "url/gurl.h"

class NavTreePage;

class NavTreeNode {
 public:
  NavTreeNode(NavTreeNode* parent,
              scoped_refptr<NavTreePage> page,
              ui::PageTransition transition_in);
  ~NavTreeNode();

  NavTreeNode* parent() { return parent_; }
  const NavTreeNode* parent() const { return parent_; }

  NavTreePage* page() { return page_.get(); }
  const NavTreePage* page() const { return page_.get(); }

  const std::vector<base::Time>& visit_times() const { return visit_times_; }
  size_t visit_count() const { return visit_times_.size(); }

  // Generally a node won't change URLs. This will happen if the page does a
  // client redirect to a different URL, keeping the NavigationEntry the same
  // but replacing the URL.
  void ChangePage(NavTreePage* new_page);

  void AddRevisit(base::Time time);

 private:
  // Non-owning (the parent owns us).
  NavTreeNode* parent_ = nullptr;

  scoped_refptr<NavTreePage> page_;

  // How the user first came to visit this node.
  // ui::PageTransition transition_in_;

  std::vector<base::Time> visit_times_;

  DISALLOW_COPY_AND_ASSIGN(NavTreeNode);
};

#endif  // CHROME_BROWSER_HISTORY_NAV_TREE_NODE_H_
