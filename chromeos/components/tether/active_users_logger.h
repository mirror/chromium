// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_COMPONENTS_TETHER_ACTIVE_USERS_LOGGER_H_
#define CHROMEOS_COMPONENTS_TETHER_ACTIVE_USERS_LOGGER_H_

#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "components/metrics/daily_event.h"

class PrefRegistrySimple;
class PrefService;

namespace chromeos {

namespace tether {

// Records when the user was last active, and then is called daily in order to
// report to UMA the last time the user was active.
class ActiveUsersLogger : public metrics::DailyEvent::Observer {
 public:
  // Registers the prefs used by this class to the given |registry|.
  static void RegisterPrefs(PrefRegistrySimple* registry);

  explicit ActiveUsersLogger(PrefService* pref_service);
  ~ActiveUsersLogger() override;

  // Record that the user was just active.
  virtual void RecordUserWasActive();

  // metrics::DailyEvent::Observer:
  void OnDailyEvent() override;

 private:
  friend class ActiveUsersLoggerTest;

  enum ActiveUser { NOT_ACTIVE = 0, ACTIVE = 1, ACTIVE_USER_MAX };

  PrefService* pref_service_;

  DISALLOW_COPY_AND_ASSIGN(ActiveUsersLogger);
};

}  // namespace tether

}  // namespace chromeos

#endif  // CHROMEOS_COMPONENTS_TETHER_ACTIVE_USERS_LOGGER_H_
