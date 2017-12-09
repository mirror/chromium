// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/history/nav_tree.h"

#include "chrome/browser/history/nav_tree_factory.h"
#include "chrome/browser/history/nav_tree_node.h"
#include "chrome/browser/history/nav_tree_page.h"
#include "chrome/browser/profiles/profile.h"
#include "components/history/content/browser/history_context_helper.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"

namespace {}  // namespace

NavTree::NavTree(Profile* profile)
    : profile_(profile), tab_strip_model_observer_(this) {}

NavTree::~NavTree() {
  log_.clear();
  visit_to_node_.clear();

  // NavTreePage leaking. This also indicates a dangling back pointer.
  DCHECK(live_pages_.empty());
}

// static
const base::Feature NavTree::kFeature{
    "NavTree", base::FEATURE_DISABLED_BY_DEFAULT,
};

// static
bool NavTree::IsEnabled() {
  return base::FeatureList::IsEnabled(kFeature);
}

void NavTree::OnNavigate(content::NavigationHandle* nav_handle) {
  if (!nav_handle->IsInMainFrame())
    return;  // For now, ignore all non-main-frame navigations.

  const GURL& url = nav_handle->GetURL();
  content::WebContents* web_contents = nav_handle->GetWebContents();
  const content::NavigationEntry* nav_entry =
      web_contents->GetController().GetLastCommittedEntry();
  int nav_entry_id = nav_entry->GetUniqueID();

  scoped_refptr<NavTreePage> page = GetOrCreatePage(url);

  NavTreeNode* referring_node = nullptr;
  history::ContextID history_context =
      history::ContextIDForWebContents(web_contents);
  history::VisitID referring_visit = visit_tracker_.GetLastVisit(
      history_context, nav_entry_id, nav_handle->GetReferrer().url);
  if (referring_visit) {
    referring_node = visit_to_node_[referring_visit];
    DCHECK(referring_node);
  }

  std::unique_ptr<NavTreeNode> node = std::make_unique<NavTreeNode>(
      referring_node, page, nav_handle->GetPageTransition());

  history::VisitID this_visit_id = next_visit_id_;
  next_visit_id_++;

  visit_tracker_.AddVisit(history_context, nav_entry_id, url, this_visit_id);
  visit_to_node_[this_visit_id] = node.get();

  if (referring_node)
    referring_node->AddTo(node.get());

  log_.push_back(std::move(node));
}

void NavTree::NavTreePageDestroyed(NavTreePage* page) {
  auto found = live_pages_.find(page->url());
  DCHECK(found != live_pages_.end());
  live_pages_.erase(found);
}

void NavTree::Shutdown() {}

scoped_refptr<NavTreePage> NavTree::GetOrCreatePage(const GURL& url) {
  NavTreePage*& page_ptr = live_pages_[url];
  if (page_ptr)
    return scoped_refptr<NavTreePage>(page_ptr);

  scoped_refptr<NavTreePage> new_page(new NavTreePage(this, url));
  page_ptr = new_page.get();
  return new_page;
}
