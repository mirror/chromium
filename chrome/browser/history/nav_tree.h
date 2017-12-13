// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_HISTORY_NAV_TREE_H_
#define CHROME_BROWSER_HISTORY_NAV_TREE_H_

#include <map>
#include <memory>
#include <vector>

#include "base/feature_list.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "chrome/browser/history/nav_tree_tab_strip_model_observer.h"
#include "components/keyed_service/core/keyed_service.h"

class GURL;
class NavJaunt;
class NavTreeNode;
class NavTreePage;
class Profile;

namespace content {
class NavigationEntry;
class NavigationHandle;
}  // namespace content

// This is an experimental class. It tracks a cross-tab, in-memory tree of
// navigations with some annotations. It is only enabled with --debug-nav-tree
class NavTree : public KeyedService {
 public:
  explicit NavTree(Profile* profile);
  ~NavTree() override;

  // Returns true if the experiment is enabled.
  static bool IsEnabled();

  Profile* profile() { return profile_; }

  // Query API -----------------------------------------------------------------

  // Returns nullptr if no node exists for it.
  NavTreeNode* NodeForNavigationEntry(const content::WebContents* contents,
                                      const content::NavigationEntry* entry);

  NavJaunt* JauntForWebContents(const content::WebContents* contents);

  // Updating API --------------------------------------------------------------

  void OnNavigate(content::NavigationHandle* nav_handle);

  void OnNavigationEntryUpdated(const content::WebContents* contents,
                                const content::NavigationEntry* entry);
  void UpdateTitleForPage(const content::WebContents* contents,
                          const content::NavigationEntry* nav_entry);

  void WebContentsDestroyed(content::WebContents* web_contents);

  // Notifies this object that a NavTreePage is being destroyed and it should
  // be removed from the live_pages_ map.
  void NavTreePageDestroyed(NavTreePage* page);

  // KeyedService:
  void Shutdown() override;

  static const base::Feature kFeature;

 private:
  scoped_refptr<NavTreePage> GetOrCreatePage(const GURL& url);

  void UpdatePageForNavigation(const content::NavigationEntry* nav_entry,
                               NavTreePage* page);

  void DebugDumpNavTreeToFile();

  Profile* profile_;
  NavTreeTabStripModelObserver tab_strip_model_observer_;

  // Map of current pages in memory. These are effectively weak pointers, with
  // NavTreePage objects notifying us when they are destroyed so this map can
  // stay up-to-date.
  std::map<GURL, NavTreePage*> live_pages_;

  // Time-sorted navigation log.
  std::vector<std::unique_ptr<NavTreeNode>> log_;

  // Navigation heads (current URLs loaded in opened tabs) indexed by
  // WebContents.
  // TODO(brettw) this needs to deal with swapping WebContents.
  std::map<content::WebContents*, NavJaunt> heads_;

  int largest_nav_entry_id_ = -1;

  DISALLOW_COPY_AND_ASSIGN(NavTree);
};

#endif  // CHROME_BROWSER_HISTORY_NAV_TREE_H_
