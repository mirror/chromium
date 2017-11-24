// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_POWER_ML_UKM_LOGGER_H_
#define CHROME_BROWSER_CHROMEOS_POWER_ML_UKM_LOGGER_H_

#include "chrome/browser/chromeos/power/ml/user_activity_logger_delegate.h"

namespace chromeos {
namespace power {
namespace ml {

class UserActivityEvent;

class UKMLogger : public UserActivityLoggerDelegate {
 public:
  // TODO(jiameng): add implementation.
  UKMLogger() = default;
  ~UKMLogger() override = default;

  // TODO(jiameng): add implementation.
  void LogActivity(const UserActivityEvent& event) override{};

 private:
  DISALLOW_COPY_AND_ASSIGN(UKMLogger);
};

}  // namespace ml
}  // namespace power
}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_POWER_ML_UKM_LOGGER_H_
