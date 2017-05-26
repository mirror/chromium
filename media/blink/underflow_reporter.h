// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BLINK_UNDERFLOW_REPORTER_H_
#define MEDIA_BLINK_UNDERFLOW_REPORTER_H_

#include "base/power_monitor/power_observer.h"
#include "base/time/time.h"
#include "media/blink/media_blink_export.h"

namespace media {

// Class for reporting metrics on underflow events.
class MEDIA_BLINK_EXPORT UnderflowReporter : base::PowerObserver {
 public:
  // Constructor for the reporter; all requested metadata must be fully known
  // before attempting construction as incorrect values will result in the wrong
  // metrics being reported.
  UnderflowReporter(bool has_audio,
                    bool has_video,
                    bool is_mse,
                    bool is_encrypted);
  ~UnderflowReporter() override;

  void OnLoaded();
  void OnUnderflow();

 private:
  // base::PowerObserver implementation.
  //
  // Since the UnderflowReporter works with TimeTicks values, which will jump
  // into the future upon a resume, we need to know when a suspend occurs so
  // we can invalidate |last_underflow_time_|.
  void OnSuspend() override;

  // Initialized during construction.
  const bool has_audio_;
  const bool has_video_;
  const bool is_mse_;
  const bool is_encrypted_;

  base::TimeTicks last_underflow_time_;

  DISALLOW_COPY_AND_ASSIGN(UnderflowReporter);
};

}  // namespace media

#endif  // MEDIA_BLINK_UNDERFLOW_REPORTER_H_
