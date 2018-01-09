// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_HISTORY_NAV_TREE_TAB_STRIP_MODEL_OBSERVER_H_
#define CHROME_BROWSER_HISTORY_NAV_TREE_TAB_STRIP_MODEL_OBSERVER_H_

#include "base/macros.h"
#include "chrome/browser/ui/browser_list_observer.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"

class NavTree;
class Profile;

// This class expects to be one-per-profile. Its job is to forward events from
// the browser and tab strip to the NavTree.
class NavTreeTabStripModelObserver : public BrowserListObserver,
                                     public TabStripModelObserver {
 public:
  explicit NavTreeTabStripModelObserver(NavTree* tree);
  ~NavTreeTabStripModelObserver() override;

 private:
  // BrowserListObserver:
  void OnBrowserAdded(Browser* browser) override;
  void OnBrowserRemoved(Browser* browser) override;

  // TabStripModelObserver:
  void TabChangedAt(content::WebContents* contents,
                    int index,
                    TabChangeType change_type) override;

  Profile* profile_;
  NavTree* tree_;  // Non-owning (it owns us).

  DISALLOW_COPY_AND_ASSIGN(NavTreeTabStripModelObserver);
};

#endif  // CHROME_BROWSER_HISTORY_NAV_TREE_TAB_STRIP_MODEL_OBSERVER_H_
