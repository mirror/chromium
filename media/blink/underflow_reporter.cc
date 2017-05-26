// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/blink/underflow_reporter.h"

#include "base/metrics/histogram_macros.h"
#include "base/power_monitor/power_monitor.h"

namespace media {

UnderflowReporter::UnderflowReporter(bool has_audio,
                                     bool has_video,
                                     bool is_mse,
                                     bool is_encrypted)
    : has_audio_(has_audio),
      has_video_(has_video),
      is_mse_(is_mse),
      is_encrypted_(is_encrypted) {
  DCHECK(has_audio_ || has_video_);
  if (base::PowerMonitor* pm = base::PowerMonitor::Get())
    pm->AddObserver(this);
}

UnderflowReporter::~UnderflowReporter() {
  if (base::PowerMonitor* pm = base::PowerMonitor::Get())
    pm->RemoveObserver(this);
}

void UnderflowReporter::OnLoaded() {
  last_underflow_time_ = base::TimeTicks::Now();
}

void UnderflowReporter::OnUnderflow() {
  const base::TimeTicks now = base::TimeTicks::Now();

  // If the last underflow time is unknown, a suspend has recently occurred, so
  // recreate our baseline from this underflow. Doing so "loses" a sample, but
  // underflow around resume is not representative of normal playback anyways.
  if (last_underflow_time_.is_null()) {
    last_underflow_time_ = now;
    return;
  }

#define REPORT_REBUFFER_TIME(metric)                               \
  UMA_HISTOGRAM_CUSTOM_TIMES(metric, now - last_underflow_time_,   \
                             base::TimeDelta::FromMilliseconds(1), \
                             base::TimeDelta::FromHours(10), 50)

  if (is_mse_)
    REPORT_REBUFFER_TIME("Media.TimeBetweenRebuffers.MSE");
  else
    REPORT_REBUFFER_TIME("Media.TimeBetweenRebuffers.SRC");

  if (is_encrypted_)
    REPORT_REBUFFER_TIME("Media.TimeBetweenRebuffers.EME");

#undef REPORT_REBUFFER_TIME

  last_underflow_time_ = base::TimeTicks();
}

void UnderflowReporter::OnSuspend() {
  // TimeTicks values will jump into the future after a resume, so clear our
  // baseline before the suspend occurs to avoid artificial inflation.
  last_underflow_time_ = base::TimeTicks();
}

}  // namespace media
