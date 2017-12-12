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
  NavTreeNode(NavTreeNode* from,
              scoped_refptr<NavTreePage> page,
              int nav_entry_id,
              ui::PageTransition transition_in);
  ~NavTreeNode();

  NavTreeNode* from() { return from_; }
  const NavTreeNode* from() const { return from_; }

  const std::vector<NavTreeNode*>& to() const { return to_; }
  void AddTo(NavTreeNode* t);

  NavTreePage* page() { return page_.get(); }
  const NavTreePage* page() const { return page_.get(); }

  int nav_entry_id() const { return nav_entry_id_; }

  ui::PageTransition transition_in() const { return transition_in_; }

  // Generally a node won't change URLs. This will happen if the page does a
  // client redirect to a different URL, keeping the NavigationEntry the same
  // but replacing the URL.
  void ChangePage(NavTreePage* new_page);

 private:
  // Non-owning. The NavTree owns the node pointers.
  NavTreeNode* from_ = nullptr;
  std::vector<NavTreeNode*> to_;  // Pages navigated to from here.

  scoped_refptr<NavTreePage> page_;
  int nav_entry_id_;

  // How the user first came to visit this node.
  ui::PageTransition transition_in_;

  DISALLOW_COPY_AND_ASSIGN(NavTreeNode);
};

#endif  // CHROME_BROWSER_HISTORY_NAV_TREE_NODE_H_
