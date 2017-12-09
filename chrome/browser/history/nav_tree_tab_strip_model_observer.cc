// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/history/nav_tree_tab_strip_model_observer.h"

#include "chrome/browser/history/nav_tree.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"

#error This thing needs to be instantiated, like by the NavTree.

NavTreeTabStripModelObserver::NavTreeTabStripModelObserver(NavTree* tree)
    : profile_(tree->profile()), tree_(tree) {
  (void)tree_;  // Avoid unused variable warning. TODO(brettw) remove this.
}

NavTreeTabStripModelObserver::~NavTreeTabStripModelObserver() {
  BrowserList* browser_list = BrowserList::GetInstance();
  for (Browser* browser : *browser_list)
    browser->tab_strip_model()->RemoveObserver(this);
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
