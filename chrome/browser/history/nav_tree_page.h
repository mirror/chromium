// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_HISTORY_NAV_TREE_PAGE_H_
#define CHROME_BROWSER_HISTORY_NAV_TREE_PAGE_H_

#include "base/containers/flat_set.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/time/time.h"
#include "url/gurl.h"

class NavTree;
class NavTreeNode;

// This is for the experimental, default-off NavTree feature.
//
// Represents a page/URL. This is independent of visits.
class NavTreePage : public base::RefCounted<NavTreePage> {
 public:
  using NodeSet = base::flat_set<NavTreeNode*>;

  // The tree must outlive this object. It will be notified when this object
  // is destroyed.
  NavTreePage(NavTree* tree, const GURL& url);

  // The URL is immutable.
  const GURL& url() const { return url_; }

  const NodeSet& nodes() const { return nodes_; }

  void AddNode(NavTreeNode* node);
  void RemoveNode(NavTreeNode* node);

 private:
  friend class base::RefCounted<NavTreePage>;

  ~NavTreePage();

  NavTree* tree_;  // Tree that owns us.
  GURL url_;

  // These are the nodes that reference this page. The nodes own the page, so
  // these are non-owning backpointers.
  NodeSet nodes_;

  DISALLOW_COPY_AND_ASSIGN(NavTreePage);
};

#endif  // CHROME_BROWSER_HISTORY_NAV_TREE_PAGE_H_
