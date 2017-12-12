// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_HISTORY_NAV_WAYPOINT_H_
#define CHROME_BROWSER_HISTORY_NAV_WAYPOINT_H_

#include <iosfwd>
#include <memory>

#include "ui/base/models/tree_node_model.h"

class NavTreeNode;

// A waypoint is an important previous page the user has been to. It references
// a node inside the NavTree. It's different than a NavTreeNode because it
// will contain a subset of the tree (only the nodes we thing are important)
// and also encodes information about why those nodes are important.
class NavWaypoint : public ui::TreeNode<NavWaypoint> {
 public:
  enum class Type {
    // A node that holds the root waypoints for a Jaunt. The node associated
    // with this waypoint will be the current page the user is looking at.
    kRoot,

    // The previously visited page.
    kBack,

    // This page was a search results page.
    kSearch,

    // A clicked search result.
    kSearchResult,

    // The page had multiple back navigations to it.
    kHub
  };

  // The node can be null for none and root types.
  NavWaypoint(Type t, NavTreeNode* node);
  ~NavWaypoint() override;

  Type type() const { return type_; }

  // Will be null for none and root types.
  NavTreeNode* node() { return node_; }
  const NavTreeNode* node() const { return node_; }
  void set_node(NavTreeNode* n) { node_ = n; }

  // TreeNode overrides.
  const base::string16& GetTitle() const override;
  void SetTitle(const base::string16& title) override;

  // Prints the waypoint hierarchy to a stream.
  void DebugPrint(std::ostream& out, int indent_spaces = 0);

  static const char* TypeToString(Type t);

 private:
  Type type_;
  NavTreeNode* node_;

  DISALLOW_COPY_AND_ASSIGN(NavWaypoint);
};

#endif  // CHROME_BROWSER_HISTORY_NAV_WAYPOINT_H_
