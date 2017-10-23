// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/trees/ukm_manager.h"

#include "services/metrics/public/cpp/ukm_builders.h"
#include "services/metrics/public/cpp/ukm_recorder.h"

namespace cc {

UkmManager::UkmManager() = default;

UkmManager::~UkmManager() = default;

void UkmManager::UpdateRecorder(std::unique_ptr<ukm::UkmRecorder> recorder,
                                ukm::SourceId source_id) {
  // If we accumulating any metrics, record them before reseting the source.
  RecordCheckerboardUkm();

  source_id_ = source_id;
  recorder_ = std::move(recorder);
}

void UkmManager::SetTreePriority(TreePriority priority) {
  const bool smoothness_mode_exited = priority_ == SMOOTHNESS_TAKES_PRIORITY &&
                                      priority != SMOOTHNESS_TAKES_PRIORITY;
  if (smoothness_mode_exited)
    RecordCheckerboardUkm();

  priority_ = priority;
}

void UkmManager::AddCheckerboardStatsForFrame(int64_t checkerboard_area,
                                              int64_t num_missing_tiles) {
  if (priority_ != SMOOTHNESS_TAKES_PRIORITY)
    return;

  checkerboarded_content_area_ += checkerboard_area;
  num_missing_tiles_ += num_missing_tiles;
  num_of_frames_++;
}

void UkmManager::RecordCheckerboardUkm() {
  if (recorder_ && num_of_frames_ > 0) {
    ukm::builders::Compositor_UserInteraction(source_id_)
        .SetCheckerboardedContentArea(checkerboarded_content_area_ /
                                      num_of_frames_)
        .SetNumMissingTiles(num_missing_tiles_ / num_of_frames_)
        .Record(recorder_.get());
  }

  checkerboarded_content_area_ = 0;
  num_missing_tiles_ = 0;
  num_of_frames_ = 0;
}

}  // namespace cc
