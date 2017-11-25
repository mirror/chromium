// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_POWER_ML_USER_ACTIVITY_LOGGER_UKM_H_
#define CHROME_BROWSER_CHROMEOS_POWER_ML_USER_ACTIVITY_LOGGER_UKM_H_

#include "base/macros.h"
#include "chrome/browser/chromeos/power/ml/user_activity_logger_delegate.h"

namespace chromeos {
namespace power {
namespace ml {

class UserActivityEvent;

class UserActivityLoggerUKM : public UserActivityLoggerDelegate {
 public:
  // TODO(jiameng): add implementation.
  UserActivityLoggerUKM() = default;
  ~UserActivityLoggerUKM() override = default;

  // TODO(jiameng): add implementation.
  void LogActivity(const UserActivityEvent& event) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(UserActivityLoggerUKM);
};

}  // namespace ml
}  // namespace power
}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_POWER_ML_USER_ACTIVITY_LOGGER_UKM_H_
