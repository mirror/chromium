// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_BITRATE_ESTIMATOR_H
#define MEDIA_BASE_BITRATE_ESTIMATOR_H

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/synchronization/lock.h"
#include "base/time/time.h"
#include "media/base/media_export.h"

namespace media {

// A BitrateEstimator estimates the stream bitrate (bits per second) with the
// frames displayed in a given |duration|. |callback| is called when one
// estimate is completed or ReportResult() is called. The estimating continues
// and callback is called repeately until it is destroyed.
//
// Frames are required to be enqueued in either display order or decoding order
// with its size and presentation time stamp. Wait for the first key frame
// enqueued to start the estimation. When enqueued in decoding order,
// the time stamps may not be in display order when B (bi-directional
// predictive) frames exist. To avoid the inaccurate estimate caused by the
// enqueue order, the BitrateEstimator waits for the enqueue of the next later
// displayed frame to stop the current estimation. For example, if the
// estimation window is from I to P1 in display order. The estimation stops
// until P2 is enqueued.
// Presentation order: I B1 B2 B3 P1 P2
// Decoding order: I P1 B1 B2 B3 P2
//
// The estimation is inaccurate if the frames are enqueued in decoding order,
// and the last B (bi-directional predictive) frame in the estimation window
// doesn't use the closest later P frame as its reference. One example is shown
// below, where B2 uses I and P2 as its reference. If the estimation window is
// from I to P2, the estimated bitrate is smaller than the actural bitrate
// because B2 is missed in the estimation.
// Presentation order: I B1 B2 P1 P2
// Decoding order: I P1 B1 P2 B2
//
// This case doesn't happen frequently since in general frames are better
// predicted by closer frames, and the implementation in this way increases
// computation complexicity and memory usage for encoders. On the other hand,
// since most B frames have much smaller size than I and P frames, the missing
// of one B frame in an estimation window (>3 seconds) may not cause large
// difference.
//
// Note: All timestamps in BitrateEstimator refer to the presentation timestamp
// of frames.
class MEDIA_EXPORT BitrateEstimator {
 public:
  enum class Status {
    kNone,
    // Estimation was completed normally.
    kOk,
    // Estimator stopped innormally (e.g. error occurred, seeking happened,
    // stream endded, etc.) before given |duration| reaches. Either 0 or the
    // available estimate is reported.
    kAborted,
  };
  using ResultCB = base::Callback<void(Status status, int bitrate)>;
  BitrateEstimator(base::TimeDelta duration, const ResultCB& callback);
  ~BitrateEstimator();

  // Enqueue one frame with its total size and the presentation |timestamp|. The
  // frames can be enqueued with display order or decoding order. See comments
  // above for details.
  void EnqueueOneFrame(int frame_bytes,
                       base::TimeDelta timestamp,
                       bool is_key_frame);

  // Force the estimator to report the current results. All stats will be reset.
  void ReportResult();

 private:
  // Enqueue one frame and update the stats. Return the size and |duration|
  // used to calculate the bitrate if ready. Otherwise return 0 and
  // base::TimeDelta().
  int64_t EnqueueFrameAndUpdate_Locked(int frame_bytes,
                                       base::TimeDelta timestamp,
                                       bool is_key_frame,
                                       base::TimeDelta* duration);

  // Return the current duration and the total size enqueued.
  int64_t GetStats_Locked(base::TimeDelta* duration);

  // Calculate the bitrate and call |callback_|.
  void CalculateAndReportBitrate(base::TimeDelta duration,
                                 int64_t data_in_bytes);

  const base::TimeDelta duration_;  // Time window to estimate the bitrate.
  const ResultCB callback_;         // Callback to be called.
  int64_t data_in_bytes_ = 0;       // Total bytes enqueued.
  base::TimeDelta min_timestamp_;   // The current minimum timestamp enqueued.
  base::TimeDelta max_timestamp_;   // The current maximum timestamp enqueued.

  // Frames are allowed to be enqueued from different threads.
  mutable base::Lock lock_;

  DISALLOW_COPY_AND_ASSIGN(BitrateEstimator);
};

}  // namespace media

#endif  // MEDIA_BASE_BITRATE_ESTIMATOR_H
