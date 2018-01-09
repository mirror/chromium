// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_HISTORY_NAV_TREE_OBSERVER_H_
#define CHROME_BROWSER_HISTORY_NAV_TREE_OBSERVER_H_

class NavTreeNode;

class NavTreeObserver {
 public:
  // If the new node is the current one loaded in the tab (the normal case),
  // is_active will be set and OnActiveNodeChanged will NOT be called.
  virtual void OnNodeAdded(NavTreeNode* node, bool is_active) = 0;

  // Indicates the currently loaded page has changed to point to an existing
  // node. When the active node changes because a new page was loaded, only
  // OnNodeAdded will be called.
  //
  // This is called after all bookkeping has been updated to point to the new
  // node.
  virtual void OnActiveNodeChanged(NavTreeNode* new_active) = 0;

  // Indicates that the data in the given node has changed. This also counts
  // changes to the page referenced by the node.
  virtual void OnNodeDataChanged(NavTreeNode* node) = 0;
};

#endif  // CHROME_BROWSER_HISTORY_NAV_TREE_OBSERVER_H_
