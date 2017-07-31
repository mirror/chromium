// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_FEATURE_ENGAGEMENT_SESSION_DURATION_UPDATER_H_
#define CHROME_BROWSER_FEATURE_ENGAGEMENT_SESSION_DURATION_UPDATER_H_

#include "chrome/browser/metrics/desktop_session_duration/desktop_session_duration_tracker.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/core/keyed_service.h"

namespace user_prefs {
class PrefRegistrySyncable;
}  // namespace user_prefs

namespace feature_engagement {

// Updates the kSessionTimeTotal pref to reflect the total amount of active
// time.
class SessionDurationUpdater
    : public metrics::DesktopSessionDurationTracker::Observer,
      public KeyedService {
 public:
  explicit SessionDurationUpdater(Profile* profile);

  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

 protected:
  SessionDurationUpdater();
  ~SessionDurationUpdater() override;

  PrefService* GetPrefs();

 private:
  // metrics::DesktopSessionDurationtracker::Observer::
  void OnSessionEnded(base::TimeDelta delta) override;

  // Assists in keeping a running total of active time for the Profile. Owned by
  // the browser.
  metrics::DesktopSessionDurationTracker* duration_tracker_;

  // Owned by Profile manager.
  Profile* const profile_;

  DISALLOW_COPY_AND_ASSIGN(SessionDurationUpdater);
};

}  // namespace feature_engagement

#endif  // CHROME_BROWSER_FEATURE_ENGAGEMENT_SESSION_DURATION_UPDATER_H_
