// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_TETHER_ACTIVE_USERS_DAILY_EVENT_OBSERVER_H_
#define CHROME_BROWSER_CHROMEOS_TETHER_ACTIVE_USERS_DAILY_EVENT_OBSERVER_H_

#include <memory>

#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "components/metrics/daily_event.h"

namespace chromeos {
namespace tether {
class ActiveUsersLogger;
}  // namespace tether
}  // namespace chromeos

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
