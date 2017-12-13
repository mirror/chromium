// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/history/nav_tree_web_contents_observer.h"

#include "chrome/browser/history/nav_tree.h"
#include "chrome/browser/history/nav_tree_factory.h"
#include "chrome/browser/profiles/profile.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(NavTreeWebContentsObserver);

NavTreeWebContentsObserver::NavTreeWebContentsObserver(
    content::WebContents* web_contents)
    : WebContentsObserver(web_contents) {
  Profile* profile =
      Profile::FromBrowserContext(web_contents->GetBrowserContext());
  tree_ = NavTreeFactory::GetForProfile(profile,
                                        ServiceAccessType::EXPLICIT_ACCESS);
}

NavTreeWebContentsObserver::~NavTreeWebContentsObserver() {}

// static
bool NavTreeWebContentsObserver::IsEnabled() {
  // This just forwards to the NavTree implementation. It's here also so the
  // code that hooks up the tab helpers doesn't need to reference extra
  // headers.
  return NavTree::IsEnabled();
}

void NavTreeWebContentsObserver::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  tree_->OnNavigate(navigation_handle);
}

void NavTreeWebContentsObserver::WebContentsDestroyed() {
  tree_->WebContentsDestroyed(web_contents());
}

void NavTreeWebContentsObserver::TitleWasSet(content::NavigationEntry* entry) {
  tree_->UpdateTitleForPage(web_contents(), entry);
}
