// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_FEATURE_ENGAGEMENT_NEW_TAB_NEW_TAB_IN_PRODUCT_HELP_SESSION_DURATION_UPDATER_H_
#define CHROME_BROWSER_FEATURE_ENGAGEMENT_NEW_TAB_NEW_TAB_IN_PRODUCT_HELP_SESSION_DURATION_UPDATER_H_

#include "chrome/browser/feature_engagement/session_duration_updater.h"
#include "components/prefs/pref_service.h"

namespace user_prefs {
class PrefRegistrySyncable;
}  // namespace user_prefs

namespace feature_engagement {

// The NewTabInProductHelpSessionDurationUpdater implements on top of
// the SessionDurationUpdater, which tracks the total amount of observed time
// across  Chrome restarts. Observed time in this context is active session time
// that occurs while there is a NewTabFeatureTracker whose active session time
// requirement has not been satisfied. This allows NewTabFeatureTracker to check
// how much active time has passed, and decide whether to show the promo
// accordingly.

class NewTabInProductHelpSessionDurationUpdater
    : public SessionDurationUpdater {
 public:
  // The methods for the observer will be called on the UI thread.
  class Observer {
   public:
    virtual ~Observer() = default;
    virtual void OnSessionEnded(base::TimeDelta total_session_time) = 0;
  };

  explicit NewTabInProductHelpSessionDurationUpdater(PrefService* pref_service);
  ~NewTabInProductHelpSessionDurationUpdater() override;

  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

  // SessionDurationUpdater:
  void OnSessionEnded(base::TimeDelta delta) override;

  DISALLOW_COPY_AND_ASSIGN(NewTabInProductHelpSessionDurationUpdater);
};

}  // namespace feature_engagement

#endif  // CHROME_BROWSER_FEATURE_ENGAGEMENT_NEW_TAB_NEW_TAB_IN_PRODUCT_HELP_SESSION_DURATION_UPDATER_H_
