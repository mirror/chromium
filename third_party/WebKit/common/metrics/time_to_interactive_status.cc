// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/WebKit/common/metrics/time_to_interactive_status.h"
#include "base/metrics/histogram_macros.h"

namespace blink {

const char kHistogramTimeToInteractiveStatus[] =
    "PageLoad.Experimental.TimeToInteractiveStatus";

void RecordTimeToInteractiveStatus(blink::TimeToInteractiveStatus status) {
  UMA_HISTOGRAM_ENUMERATION(blink::kHistogramTimeToInteractiveStatus, status,
                            blink::TIME_TO_INTERACTIVE_LAST_ENTRY);
}

}  // namespace blink
