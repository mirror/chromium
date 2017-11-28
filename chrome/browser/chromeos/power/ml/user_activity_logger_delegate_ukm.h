// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_POWER_ML_USER_ACTIVITY_LOGGER_DELEGATE_UKM_H_
#define CHROME_BROWSER_CHROMEOS_POWER_ML_USER_ACTIVITY_LOGGER_DELEGATE_UKM_H_

#include <vector>

#include "base/macros.h"
#include "chrome/browser/chromeos/power/ml/user_activity_logger_delegate.h"
#include "services/metrics/public/cpp/ukm_recorder.h"

namespace chromeos {
namespace power {
namespace ml {

class UserActivityEvent;

// Places |original_value| into buckets of size 5, i.e. if |original_value| is
// in [0, 5), we map it 0; if it is in [5, 10), we map it to 5 etc.
// |original_value| should be in the range of [0, 100].
int BucketEveryFivePercents(int original_value);

class UserActivityLoggerDelegateUkm : public UserActivityLoggerDelegate {
 public:
  UserActivityLoggerDelegateUkm();
  ~UserActivityLoggerDelegateUkm() override;

  // Populates |source_ids_| with all open tabs' URLs' source ids.
  void UpdateOpenTabsURLs() override;

  // Sends |event| and |source_ids_| to UKM. Also clears |source_ids_|.
  void LogActivity(const UserActivityEvent& event) override;

 private:
  ukm::UkmRecorder* ukm_recorder_;  // not owned

  // Source IDs of open tabs' URLs.
  std::vector<ukm::SourceId> source_ids_;

  DISALLOW_COPY_AND_ASSIGN(UserActivityLoggerDelegateUkm);
};

}  // namespace ml
}  // namespace power
}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_POWER_ML_USER_ACTIVITY_LOGGER_DELEGATE_UKM_H_
