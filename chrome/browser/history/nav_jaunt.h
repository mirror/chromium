// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_HISTORY_NAV_JAUNT_H_
#define CHROME_BROWSER_HISTORY_NAV_JAUNT_H_

#include <map>
#include <memory>

#include "base/macros.h"
#include "chrome/browser/history/nav_waypoint.h"
#include "ui/base/models/tree_node_model.h"

class NavWaypoint;

// A jaunt is a user journey consisting of a sequence of waypoints relative
// to a given root. Ideally this will represent the important points that the
// user has navigated to prior to this page.
//
// This class implements TreeModel. All TreeModelNodes from this class can
// be casted to NavWaypoints for more details.
class NavJaunt : public ui::TreeNodeModel<NavWaypoint> {
 public:
  enum ChangeType {
    kNew,  // Navigated to a new page.
    kOld   // Renavigatied to an existing node.
  };

  NavJaunt();
  ~NavJaunt() override;

  // Callers will want the Root waypoint. Its getter is on the TreeNodeModel:
  //   NavWaypoint* GetRoot();

  // Helper to get the NavTreeNode associated with the root. This is the
  // current page.
  NavTreeNode* root_node() { return GetRoot()->node(); }
  const NavTreeNode* root_node() const {
    return const_cast<NavJaunt*>(this)->GetRoot()->node();
  }

  // Updates the current page the user is looking at. This will update the
  // waypoints and notify observers.
  void SetCurrent(NavTreeNode* current, ChangeType change);

 private:
  void UpdateBackNode();

  // Adds the root node as a new search or search result if necessary. Returns
  // true if it was that type.
  bool AddSearchIfNecessary();
  bool AddSearchResultIfNecessary();

  // When true, there is a "back" node which is the first child of the root
  // node.
  bool has_back_node_ = false;

  // Record of all search nodes in this Jaunt, indexed by the search terms.
  std::map<std::string, NavWaypoint*> searches_;

  DISALLOW_COPY_AND_ASSIGN(NavJaunt);
};

#endif  // CHROME_BROWSER_HISTORY_NAV_JAUNT_H_
