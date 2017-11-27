// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_POWER_ML_USER_ACTIVITY_LOGGER_UKM_H_
#define CHROME_BROWSER_CHROMEOS_POWER_ML_USER_ACTIVITY_LOGGER_UKM_H_

#include "base/macros.h"
#include "chrome/browser/chromeos/power/ml/user_activity_logger_delegate.h"
#include "services/metrics/public/cpp/ukm_recorder.h"

namespace chromeos {
namespace power {
namespace ml {

class UserActivityEvent;

class UserActivityLoggerUKM : public UserActivityLoggerDelegate {
 public:
  UserActivityLoggerUKM();
  ~UserActivityLoggerUKM() override;

  // Populates |source_ids_| with all open tabs' URLs' source ids.
  void GetOpenTabsURLs();

  // Sends |event| and |source_ids_| to UKM. Also clears |source_ids_|.
  void LogActivity(const UserActivityEvent& event) override;

 private:
  ukm::UkmRecorder* ukm_recorder_;  // not owned

  // Source ids of open tabs' URLs.
  std::vector<ukm::SourceId> source_ids_;

  DISALLOW_COPY_AND_ASSIGN(UserActivityLoggerUKM);
};

}  // namespace ml
}  // namespace power
}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_POWER_ML_USER_ACTIVITY_LOGGER_UKM_H_
