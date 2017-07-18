// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/tether/active_users_daily_event_observer.h"

#include "chromeos/components/tether/active_users_logger.h"

ActiveUsersDailyEventObserver::ActiveUsersDailyEventObserver(
    chromeos::tether::ActiveUsersLogger* active_users_logger)
    : active_users_logger_(active_users_logger) {}

ActiveUsersDailyEventObserver::~ActiveUsersDailyEventObserver() {}

void ActiveUsersDailyEventObserver::OnDailyEvent() {
  active_users_logger_->ReportUserActivity();
}
