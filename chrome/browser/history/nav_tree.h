// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_HISTORY_NAV_TREE_H_
#define CHROME_BROWSER_HISTORY_NAV_TREE_H_

#include <map>
#include <memory>

#include "base/feature_list.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "components/history/core/browser/visit_tracker.h"
#include "components/keyed_service/core/keyed_service.h"

class GURL;
class NavTreeNode;
class NavTreePage;

namespace content {
class NavigationHandle;
}

namespace history {
struct HistoryAddPageArgs;
}

// This is an experimental class. It tracks a cross-tab, in-memory tree of
// navigations with some annotations. It is only enabled with --debug-nav-tree
class NavTree : public KeyedService {
 public:
  NavTree();
  ~NavTree() override;

  // Returns true if the experiment is enabled.
  static bool IsEnabled();

  // If the experiment is enabled, udpates the NavTree for the given
  // navigation. Otherwise does nothing so it is safe to unconditionally call.
  static void UpdateForNavigation(content::NavigationHandle* nav_handle,
                                  const history::HistoryAddPageArgs& args);

  void OnNavigate(content::NavigationHandle* nav_handle,
                  const history::HistoryAddPageArgs& args);

  // Notifies this object that a NavTreePage is being destroyed and it should
  // be removed from the live_pages_ map.
  void NavTreePageDestroyed(NavTreePage* page);

  // KeyedService:
  void Shutdown() override;

  static const base::Feature kFeature;

 private:
  scoped_refptr<NavTreePage> GetOrCreatePage(const GURL& url);

  // Map of current pages in memory. These are effectively weak pointers, with
  // NavTreePage objects notifying us when they are destroyed so this map can
  // stay up-to-date.
  std::map<GURL, NavTreePage*> live_pages_;

  // Time-sorted navigation log.
  std::vector<std::unique_ptr<NavTreeNode>> log_;

  // This re-uses the history visit tracker to track referrers, but that
  // produces "visit IDs". For now, use this visit ID in another map to allow
  // node lookup. If we start doing this in production code, the visit tracker
  // should be generalized so two maps aren't necessary here.
  history::VisitID next_visit_id_ = 1;  // 0 means "no visit".
  history::VisitTracker visit_tracker_;
  std::map<history::VisitID, NavTreeNode*> visit_to_node_;

  DISALLOW_COPY_AND_ASSIGN(NavTree);
};

#endif  // CHROME_BROWSER_HISTORY_NAV_TREE_H_
