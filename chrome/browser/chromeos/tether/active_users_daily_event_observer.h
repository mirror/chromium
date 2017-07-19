// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_TETHER_ACTIVE_USERS_DAILY_EVENT_OBSERVER_H_
#define CHROME_BROWSER_CHROMEOS_TETHER_ACTIVE_USERS_DAILY_EVENT_OBSERVER_H_

#include "base/macros.h"
#include "components/metrics/daily_event.h"

namespace chromeos {
namespace tether {
class ActiveUsersLogger;
}  // namespace tether
}  // namespace chromeos

// Is called daily, and calls on chromeos::tether::ActiveUsersLogger to report
// user activity to UMA.
class ActiveUsersDailyEventObserver : public metrics::DailyEvent::Observer {
 public:
  explicit ActiveUsersDailyEventObserver(
      chromeos::tether::ActiveUsersLogger* active_users_logger);
  ~ActiveUsersDailyEventObserver() override;

  // metrics::DailyEvent::Observer:
  void OnDailyEvent() override;

 private:
  chromeos::tether::ActiveUsersLogger* active_users_logger_;

  DISALLOW_COPY_AND_ASSIGN(ActiveUsersDailyEventObserver);
};

#endif  // CHROME_BROWSER_CHROMEOS_TETHER_ACTIVE_USERS_DAILY_EVENT_OBSERVER_H_
