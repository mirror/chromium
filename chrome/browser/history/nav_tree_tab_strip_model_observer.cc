// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/history/nav_tree_tab_strip_model_observer.h"

#include "chrome/browser/history/nav_tree.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"

NavTreeTabStripModelObserver::NavTreeTabStripModelObserver(NavTree* tree)
    : profile_(tree->profile()), tree_(tree) {
  BrowserList* browser_list = BrowserList::GetInstance();
  browser_list->AddObserver(this);
  for (Browser* browser : *browser_list) {
    if (browser->profile() == profile_)
      browser->tab_strip_model()->RemoveObserver(this);
  }
}

NavTreeTabStripModelObserver::~NavTreeTabStripModelObserver() {
  BrowserList* browser_list = BrowserList::GetInstance();
  for (Browser* browser : *browser_list) {
    if (browser->profile() == profile_)
      browser->tab_strip_model()->RemoveObserver(this);
  }
  browser_list->RemoveObserver(this);
}

void NavTreeTabStripModelObserver::OnBrowserAdded(Browser* browser) {
  if (browser->profile() == profile_)
    browser->tab_strip_model()->AddObserver(this);
}

void NavTreeTabStripModelObserver::OnBrowserRemoved(Browser* browser) {
  if (browser->profile() == profile_)
    browser->tab_strip_model()->RemoveObserver(this);
}

void NavTreeTabStripModelObserver::TabChangedAt(content::WebContents* contents,
                                                int index,
                                                TabChangeType change_type) {
  // This tab strip "TabChangedAt" seems to be the only way to get notified of
  // a favicon change. This may get called more often than we need.
  // TODO(brettw) Consider adding a more official WebContents observer
  // notification for this.
  auto* nav_entry = contents->GetController().GetVisibleEntry();
  if (nav_entry)
    tree_->OnNavigationEntryUpdated(contents, nav_entry);
}
