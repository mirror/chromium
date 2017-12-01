// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/browsing_data/navigation_entry_remover.h"

#include "build/build_config.h"
#include "chrome/browser/browsing_data/browsing_data_helper.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
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

// Returns true if entry is in |urls|.
bool OriginMatcher(const std::set<GURL>& urls,
                   const content::NavigationEntry& entry) {
  return urls.find(entry.GetURL()) != urls.end();
}

// Returns true if the url of |entry| has a web scheme and was visited between
// |begin| and |end|.
bool TimeRangeMatcher(base::Time begin,
                      base::Time end,
                      const content::NavigationEntry& entry) {
  return BrowsingDataHelper::HasWebScheme(entry.GetURL()) &&
         begin <= entry.GetTimestamp() && end >= entry.GetTimestamp();
}

template <typename TabList>
void PruneNavigationEntries(
    TabList* tab_list,
    int tab_count,
    const content::NavigationController::DeletionPredicate& predicate) {
  for (int i = 0; i < tab_count; i++) {
    content::WebContents* web_contents = tab_list->GetWebContentsAt(i);
    if (!web_contents->GetController().CanPruneAllButLastCommitted())
      continue;
    web_contents->GetController().PruneNavigationEntries(predicate);
  }
}

}  // namespace

namespace browsing_data {

// TODO(dullweber): Also remove navigations on IOS?
void RemoveNavigationEntries(Profile* profile,
                             const history::DeletionTimeRange& time_range,
                             const history::URLRows& deleted_rows) {
  content::NavigationController::DeletionPredicate predicate;
  if (time_range.is_valid()) {
    // TODO(dullweber): Also use this for time ranges. How to get them?
    predicate = base::BindRepeating(&TimeRangeMatcher, time_range.begin(),
                                    time_range.end());
  } else {
    std::set<GURL> urls;
    for (const history::URLRow& row : deleted_rows) {
      urls.insert(row.url());
    }
    predicate = base::BindRepeating(&OriginMatcher, std::move(urls));
  }

  DCHECK(profile->GetProfileType() == Profile::ProfileType::REGULAR_PROFILE);
#if defined(OS_ANDROID)
  for (auto it = TabModelList::begin(); it != TabModelList::end(); ++it) {
    TabModel* tab_model = *it;
    if (tab_model->GetProfile() == profile) {
      PruneNavigationEntries(tab_model, tab_model->GetTabCount(), predicate);
    }
  }
#else
  for (Browser* browser : *BrowserList::GetInstance()) {
    TabStripModel* tab_strip = browser->tab_strip_model();
    if (browser->profile() == profile) {
      PruneNavigationEntries(tab_strip, tab_strip->count(), predicate);
    }
  }
#endif
}

}  // namespace browsing_data
