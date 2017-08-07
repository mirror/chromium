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
// new tab button.
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
  // Returns whether or not the promo should be displayed.
  bool ShouldShowPromo();

  // FeatureTracker:
  void OnSessionTimeMet() override;

 protected:
  // Alternate constructor to support unit testing.
  explicit NewTabTracker(SessionDurationUpdater* session_duration_updater);
  ~NewTabTracker() override;

 private:
  // FeatureTracker:
  int GetSessionTimeRequiredToShowInMinutes() override;

  // Calls the NewTabPromo to be shown.
  void ShowPromo();

  // Returns whether the NewTabInProductHelp field trial is enabled.
  bool IsIPHNewTabEnabled();

  DISALLOW_COPY_AND_ASSIGN(NewTabTracker);
};

}  // namespace feature_engagement

#endif  // CHROME_BROWSER_FEATURE_ENGAGEMENT_NEW_TAB_NEW_TAB_TRACKER_H_
