// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_HISTORY_NAV_TREE_WEB_CONTENTS_OBSERVER_H_
#define CHROME_BROWSER_HISTORY_NAV_TREE_WEB_CONTENTS_OBSERVER_H_

#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

class NavTree;

// An adapter class between WebContents and the NavTree.
class NavTreeWebContentsObserver
    : public content::WebContentsObserver,
      public content::WebContentsUserData<NavTreeWebContentsObserver> {
 public:
  ~NavTreeWebContentsObserver() override;

  // Returns true if the experiment is enabled.
  static bool IsEnabled();

 private:
  explicit NavTreeWebContentsObserver(content::WebContents* web_contents);
  friend class content::WebContentsUserData<NavTreeWebContentsObserver>;

  // content::WebContentsObserver implementation.
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;
  void WebContentsDestroyed() override;
  void TitleWasSet(content::NavigationEntry* entry) override;

  NavTree* tree_ = nullptr;  // Non-owning.

  DISALLOW_COPY_AND_ASSIGN(NavTreeWebContentsObserver);
};

#endif  // CHROME_BROWSER_HISTORY_NAV_TREE_WEB_CONTENTS_OBSERVER_H_
