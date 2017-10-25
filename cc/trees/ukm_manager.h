// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_TREES_UKM_MANAGER_H_
#define CC_TREES_UKM_MANAGER_H_

#include "base/numerics/checked_math.h"
#include "cc/cc_export.h"
#include "cc/tiles/tile_priority.h"
#include "services/metrics/public/cpp/ukm_source_id.h"

namespace ukm {
class UkmRecorder;
}  // namespace ukm

namespace cc {

class CC_EXPORT UkmManager {
 public:
  UkmManager();
  ~UkmManager();

  void UpdateRecorder(std::unique_ptr<ukm::UkmRecorder> recorder,
                      ukm::SourceId source_id);

  void SetUserInteractionInProgress(bool in_progress);
  void AddCheckerboardStatsForFrame(int64_t checkerboard_area,
                                    int64_t num_missing_tiles);

  ukm::UkmRecorder* recorder_for_testing() { return recorder_.get(); }

 private:
  void RecordCheckerboardUkm();

  bool user_interaction_in_progress_ = false;
  int64_t checkerboarded_content_area_ = 0;
  int64_t num_missing_tiles_ = 0;
  int64_t num_of_frames_ = 0;

  ukm::SourceId source_id_;
  std::unique_ptr<ukm::UkmRecorder> recorder_;
};

}  // namespace cc

#endif  // CC_TREES_UKM_MANAGER_H_
