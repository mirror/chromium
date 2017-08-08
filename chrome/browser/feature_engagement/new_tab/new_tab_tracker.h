// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_FEATURE_ENGAGEMENT_NEW_TAB_NEW_TAB_TRACKER_H_
#define CHROME_BROWSER_FEATURE_ENGAGEMENT_NEW_TAB_NEW_TAB_TRACKER_H_

#include "chrome/browser/feature_engagement/feature_tracker.h"

#include "chrome/browser/feature_engagement/session_duration_updater.h"
#include "chrome/browser/feature_engagement/session_duration_updater_factory.h"

namespace feature_engagement {

// The NewTabTracker provides a backend for displaying in-product help for the
// new tab button. NewTabTracker is the interface through which the event
// constants for the NewTab feature can be be altered. Once all of the event
// constants are met, NewTabTracker calls for the NewTabPromo to be shown, along
// with recording when the NewTabPromo is dimissed. The requirements to show
// the NewTabPromo are as follows:
//
// - At least two hours of observed session time have elapsed.
// - The user has never opened another tab through any means.
// - The user has navigated away from the start home screen.
// - The omnibox is in focus, which implies the user is intending on navigating
//   to a new page.
class NewTabTracker : public FeatureTracker {
 public:
  NewTabTracker(Tracker* tracker,
                SessionDurationUpdater* session_duration_updater);

  // Clears the flag for whether there is any in-product help being displayed.
  void DismissNewTabTracker();
  // Alerts the new tab tracker that a new tab was opened.
  void OnNewTabOpened();
  // Alerts the new tab tracker that the omnibox has been used.
  void OnOmniboxNavigation();
  // Checks if the promo should be displayed since the omnibox is on focus.
  void OnOmniboxFocused();

  // Public to support unit tests:
  // FeatureTracker:
  void OnSessionTimeMet() override;
  // Returns whether or not the promo should be displayed.
  bool ShouldShowPromo();

 protected:
  // Alternate constructor to support unit testing.
  explicit NewTabTracker(SessionDurationUpdater* session_duration_updater);
  ~NewTabTracker() override;

 private:
  // FeatureTracker:
  int GetSessionTimeRequiredToShowInMinutes() override;

  // Calls the NewTabPromo to be shown.
  void ShowPromo();

  DISALLOW_COPY_AND_ASSIGN(NewTabTracker);
};

}  // namespace feature_engagement

#endif  // CHROME_BROWSER_FEATURE_ENGAGEMENT_NEW_TAB_NEW_TAB_TRACKER_H_
