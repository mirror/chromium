// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/history/nav_tree.h"

#include "chrome/browser/history/nav_tree_factory.h"
#include "chrome/browser/history/nav_tree_node.h"
#include "chrome/browser/history/nav_tree_page.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"

namespace {}  // namespace

NavTree::NavTree() {}
NavTree::~NavTree() {}

// static
const base::Feature NavTree::kFeature{
    "NavTree", base::FEATURE_DISABLED_BY_DEFAULT,
};

// static
bool NavTree::IsEnabled() {
  return base::FeatureList::IsEnabled(kFeature);
}

// static
void NavTree::UpdateForNavigation(content::NavigationHandle* nav_handle,
                                  const history::HistoryAddPageArgs& args) {
  if (!IsEnabled())
    return;

  if (!navigation_handle->IsInMainFrame())
    return;  // For now, ignore all non-main-frame navigations.

  content::WebContents* contents = nav_handle->GetWebContents();
  Profile* profile = Profile::FromBrowserContext(contents->GetBrowserContext());
  NavTree* tree = NavTreeFactory::GetForProfile(
      profile, ServiceAccessType::EXPLICIT_ACCESS);
  tree->OnNavigate(navigation_handle, args);
}

void NavTree::OnNavigate(content::NavigationHandle* nav_handle,
                         const history::HistoryAddPageArgs& args) {
  scoped_refptr<NavTreePage> page =
      GetOrCreatePage(navigation_handle->GetURL());

  std::unique_ptr<NavTreeNode> node = std::make_unique<NavTreeNode>(
      parent, page, nav_handle->GetPageTransition());
}

void NavTree::NavTreePageDestroyed(NavTreePage* page) {}

void NavTree::Shutdown() {}

scoped_refptr<NavTreePage> NavTree::GetOrCreatePage(const GURL& url) {
  NavTreePage*& page_ptr = live_pages_[url];
  if (page_ptr)
    return scoped_refptr<NavTreePage>(page_ptr);

  scoped_refptr<NavTreePage> new_page(new NavTreePage(url));
  page_ptr = new_page.get();
  return new_page;
}
