// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/browsing_data/navigation_entry_remover.h"

#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/web_contents.h"

#if defined(OS_ANDROID)
#include "chrome/browser/android/tab_android.h"
#include "chrome/browser/ui/android/tab_model/tab_model.h"
#include "chrome/browser/ui/android/tab_model/tab_model_list.h"
#else
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#endif

namespace {

template <typename TabList>
void PruneNavigationEntries(TabList* tab_list,
                            int tab_count,
                            bool all_history,
                            const std::set<GURL>& origins) {
  for (int i = 0; i < tab_count; i++) {
    content::WebContents* web_contents = tab_list->GetWebContentsAt(i);
    if (web_contents->GetController().CanPruneAllButLastCommitted()) {
      if (all_history)
        web_contents->GetController().PruneAllButLastCommitted();
      else
        web_contents->GetController().PruneNavigationEntries(origins);
    }
  }
}

}  // namespace

namespace browsing_data {

// TODO(dullweber): Also remove navigations on IOS?
void RemoveNavigationEntries(Profile* profile,
                             bool all_history,
                             const history::URLRows& deleted_rows) {
  if (deleted_rows.empty())
    return;
  std::set<GURL> urls;
  if (!all_history) {
    for (const history::URLRow& row : deleted_rows) {
      urls.insert(row.url());
      LOG(ERROR) << "history:" << row.url();
    }
  }

  DCHECK(profile->GetProfileType() == Profile::ProfileType::REGULAR_PROFILE);
#if defined(OS_ANDROID)
  for (auto it = TabModelList::begin(); it != TabModelList::end(); ++it) {
    TabModel* tab_model = *it;
    if (tab_model->GetProfile() == profile) {
      PruneNavigationEntries(tab_model, tab_model->GetTabCount(), all_history,
                             urls);
    }
  }
#else
  for (Browser* browser : *BrowserList::GetInstance()) {
    TabStripModel* tab_strip = browser->tab_strip_model();
    if (browser->profile() == profile) {
      PruneNavigationEntries(tab_strip, tab_strip->count(), all_history, urls);
    }
  }
#endif
}

}  // namespace browsing_data
