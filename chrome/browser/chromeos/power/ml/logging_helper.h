// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_POWER_ML_LOGGING_HELPER_H_
#define CHROME_BROWSER_CHROMEOS_POWER_ML_LOGGING_HELPER_H_

namespace chromeos {
namespace power {
namespace ml {

class UserActivityEvent;

// Abstract calss to log UserActivityEvent to UKM.
class LoggingHelper {
 public:
  virtual ~LoggingHelper() = default;
  // Log user activity event.
  virtual void LogActivity(const UserActivityEvent&) = 0;
};

}  // namespace ml
}  // namespace power
}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_POWER_ML_LOGGING_HELPER_H_
