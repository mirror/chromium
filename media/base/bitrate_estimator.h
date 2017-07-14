// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_BITRATE_ESTIMATOR_H
#define MEDIA_BASE_BITRATE_ESTIMATOR_H

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/synchronization/lock.h"
#include "base/time/tick_clock.h"
#include "base/time/time.h"
#include "media/base/media_export.h"

namespace media {

// A BitrateEstimator estimates how many bits are enqueued per second. The
// estimation starts since the BitrateEstimator is created or Reset() is
// called. The GetBitrate() call ends the estimation and returns the result.
class MEDIA_EXPORT BitrateEstimator {
 public:
  BitrateEstimator();
  ~BitrateEstimator();

  // Enqueue one frame with its size.
  void EnqueueOneFrame(int frame_bytes);

  // Resets the estimation. The previous stats are discarded.
  void Reset();

  // Returns the estimated bitrate since the BitrateEstimator is created or the
  // latest Reset() is called.
  double GetBitrate();

 private:
  friend class BitrateEstimatorTest;  // Friend class for unit tests.

  std::unique_ptr<base::TickClock> clock_;
  int64_t data_in_bytes_ = 0;   // Total bytes enqueued.
  base::TimeTicks start_time_;  // The time that estimation starts.

  // Frames are allowed to be enqueued from different threads.
  mutable base::Lock lock_;

  DISALLOW_COPY_AND_ASSIGN(BitrateEstimator);
};

}  // namespace media

#endif  // MEDIA_BASE_BITRATE_ESTIMATOR_H
