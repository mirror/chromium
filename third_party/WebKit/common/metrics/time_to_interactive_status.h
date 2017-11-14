// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_WEBKIT_COMMON_METRICS_TIME_TO_INTERACTIVE_STATUS_H
#define THIRD_PARTY_WEBKIT_COMMON_METRICS_TIME_TO_INTERACTIVE_STATUS_H

#include "third_party/WebKit/common/common_export.h"

namespace blink {

extern const char BLINK_COMMON_EXPORT kHistogramTimeToInteractiveStatus[];

enum TimeToInteractiveStatus {
  // Time to Interactive recorded succesfully.
  TIME_TO_INTERACTIVE_RECORDED = 0,

  // Reasons for not recording Time to Interactive:
  // Main thread and network quiescence reached, but the user backgrounded the
  // page at least once before reaching quiescence.
  TIME_TO_INTERACTIVE_BACKGROUNDED = 1,

  // Main thread and network quiescence reached, but there was a non-mouse-move
  // user input that hit the renderer main thread between navigation start the
  // interactive time, so the detected interactive time is inaccurate. Note that
  // Time to Interactive is not invalidated if the user input is after
  // interactive time, but before quiescence windows are detected.
  TIME_TO_INTERACTIVE_USER_INTERACTION_BEFORE_INTERACTIVE = 2,

  // User left page before main thread and network quiescence, but after First
  // Meaningful Paint.
  TIME_TO_INTERACTIVE_DID_NOT_REACH_QUIESCENCE = 3,

  // User left page before First Meaningful Paint happened, but after First
  // Paint.
  TIME_TO_INTERACTIVE_DID_NOT_REACH_FIRST_MEANINGFUL_PAINT = 4,

  TIME_TO_INTERACTIVE_LAST_ENTRY
};

void BLINK_COMMON_EXPORT
RecordTimeToInteractiveStatus(blink::TimeToInteractiveStatus status);

}  // namespace blink

#endif  // THIRD_PARTY_WEBKIT_COMMON_METRICS_TIME_TO_INTERACTIVE_STATUS_H
