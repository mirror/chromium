// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/components/tether/active_users_logger.h"

#include "base/metrics/histogram_macros.h"
#include "chromeos/components/tether/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace chromeos {

namespace tether {

// static
void ActiveUsersLogger::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterInt64Pref(prefs::kUserLastActiveTimestamp, 0);
}

ActiveUsersLogger::ActiveUsersLogger(PrefService* pref_service)
    : pref_service_(pref_service) {}

ActiveUsersLogger::~ActiveUsersLogger() {}

void ActiveUsersLogger::RecordUserWasActive() {
  base::Time now = base::Time::Now();
  pref_service_->SetInt64(prefs::kUserLastActiveTimestamp,
                          now.ToInternalValue());
}

void ActiveUsersLogger::ReportUserActivity() {
  base::Time now = base::Time::Now();
  base::Time user_last_active = base::Time::FromInternalValue(
      pref_service_->GetInt64(prefs::kUserLastActiveTimestamp));
  int days_elapsed = (now - user_last_active).InDays();

  if (days_elapsed <= 1) {
    UMA_HISTOGRAM_ENUMERATION("InstantTethering.ActiveUsers.1DA",
                              ActiveUser::ACTIVE, ActiveUser::ACTIVE_USER_MAX);
  }

  if (days_elapsed <= 7) {
    UMA_HISTOGRAM_ENUMERATION("InstantTethering.ActiveUsers.7DA",
                              ActiveUser::ACTIVE, ActiveUser::ACTIVE_USER_MAX);
  }

  if (days_elapsed <= 14) {
    UMA_HISTOGRAM_ENUMERATION("InstantTethering.ActiveUsers.14DA",
                              ActiveUser::ACTIVE, ActiveUser::ACTIVE_USER_MAX);
  }

  if (days_elapsed <= 28) {
    UMA_HISTOGRAM_ENUMERATION("InstantTethering.ActiveUsers.28DA",
                              ActiveUser::ACTIVE, ActiveUser::ACTIVE_USER_MAX);
  }
}

}  // namespace tether

}  // namespace chromeos
