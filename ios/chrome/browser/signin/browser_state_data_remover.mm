// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/signin/browser_state_data_remover.h"

#include <memory>

#include "base/logging.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "components/prefs/pref_service.h"
#include "components/signin/core/browser/signin_pref_names.h"
#include "ios/chrome/browser/bookmarks/bookmarks_utils.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#import "ios/chrome/browser/browsing_data/browsing_data_removal_controller.h"
#include "ios/chrome/browser/reading_list/reading_list_remover_helper.h"
#import "ios/chrome/browser/ui/chrome_web_view_factory.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
const int kRemoveAllDataMask = ~0;
}

BrowserStateDataRemover::~BrowserStateDataRemover() {}

// static
void BrowserStateDataRemover::ClearData(
    ios::ChromeBrowserState* browser_state,
    BrowsingDataRemovalController* controller,
    ProceduralBlock completion) {
  DCHECK(browser_state);
  DCHECK(controller);

  // The user just changed the account and chose to clear the previously
  // existing data. As browsing data is being cleared, it is fine to clear the
  // last username, as there will be no data to be merged.
  (new BrowserStateDataRemover(browser_state, completion))
      ->RemoveBrowserStateData(controller);
}

BrowserStateDataRemover::BrowserStateDataRemover(
    ios::ChromeBrowserState* browser_state,
    ProceduralBlock completion)
    : browser_state_(browser_state), completion_block_([completion copy]) {
  callback_subscription_ =
      IOSChromeBrowsingDataRemover::RegisterOnBrowsingDataRemovedCallback(
          base::Bind(&BrowserStateDataRemover::NotifyWithDetails,
                     base::Unretained(this)));
}

void BrowserStateDataRemover::RemoveBrowserStateData(
    BrowsingDataRemovalController* controller) {
  [controller
      removeBrowsingDataFromBrowserState:browser_state_
                                    mask:kRemoveAllDataMask
                              timePeriod:browsing_data::TimePeriod::ALL_TIME
                       completionHandler:nil];

  base::Time beginDeleteTime = browsing_data::CalculateBeginDeleteTime(
      browsing_data::TimePeriod::ALL_TIME);
  [ChromeWebViewFactory clearExternalCookies:browser_state_
                                    fromTime:beginDeleteTime
                                      toTime:base::Time::Max()];
}

void BrowserStateDataRemover::NotifyWithDetails(
    const IOSChromeBrowsingDataRemover::NotificationDetails& details) {
  // Remove bookmarks and Reading List entries once all browsing data was
  // removed. Removal of browsing data waits for the bookmark model to be
  // loaded, so there should be no issue calling the function here.
  CHECK(RemoveAllUserBookmarksIOS(browser_state_))
      << "Failed to remove all user bookmarks.";
  reading_list_remover_helper_ =
      std::make_unique<reading_list::ReadingListRemoverHelper>(browser_state_);
  reading_list_remover_helper_->RemoveAllUserReadingListItemsIOS(
      base::Bind(&BrowserStateDataRemover::ReadingListCleaned,
                 base::Unretained(this), details));
}

void BrowserStateDataRemover::ReadingListCleaned(
    const IOSChromeBrowsingDataRemover::NotificationDetails& details,
    bool reading_list_cleaned) {
  CHECK(reading_list_cleaned)
      << "Failed to remove all user reading list items.";

  if (details.removal_mask != kRemoveAllDataMask) {
    NOTREACHED() << "Unexpected partial remove browsing data request "
                 << "(removal mask = " << details.removal_mask << ")";
    return;
  }

  browser_state_->GetPrefs()->ClearPref(prefs::kGoogleServicesLastAccountId);
  browser_state_->GetPrefs()->ClearPref(prefs::kGoogleServicesLastUsername);

  if (completion_block_)
    completion_block_();

  base::ThreadTaskRunnerHandle::Get()->DeleteSoon(FROM_HERE, this);
}
