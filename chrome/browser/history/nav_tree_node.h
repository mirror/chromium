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

class NavTreeNode : public base::RefCounted<NavTreeNode> {
 public:
  using ToVector = std::vector<scoped_refptr<NavTreeNode>>;

  NavTreeNode(NavTreeNode* from,
              scoped_refptr<NavTreePage> page,
              int nav_entry_id,
              ui::PageTransition transition_in,
              base::Time time);

  NavTreeNode* from() { return from_; }
  const NavTreeNode* from() const { return from_; }

  const ToVector& to() const { return to_; }
  void AddTo(scoped_refptr<NavTreeNode> t);

  NavTreePage* page() { return page_.get(); }
  const NavTreePage* page() const { return page_.get(); }

  int nav_entry_id() const { return nav_entry_id_; }

  ui::PageTransition transition_in() const { return transition_in_; }

  base::Time time() const { return time_; }

  // Generally a node won't change URLs. This will happen if the page does a
  // client redirect to a different URL, keeping the NavigationEntry the same
  // but replacing the URL.
  void ChangePage(NavTreePage* new_page);

 private:
  friend class base::RefCounted<NavTreeNode>;

  ~NavTreeNode();

  // Non-owning back pointer to the owner of this node. Can be null for the
  // origin of a navigation, and can also be cleared if somebody owns a
  // reference to this node but the "from" node was released.
  NavTreeNode* from_ = nullptr;

  ToVector to_;  // Pages navigated from here.

  scoped_refptr<NavTreePage> page_;
  int nav_entry_id_;

  // How the user first came to visit this node.
  ui::PageTransition transition_in_;

  base::Time time_;

  DISALLOW_COPY_AND_ASSIGN(NavTreeNode);
};

#endif  // CHROME_BROWSER_HISTORY_NAV_TREE_NODE_H_
