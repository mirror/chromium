// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/timing/SubTaskAttribution.h"

#include <utility>

namespace blink {

SubTaskAttribution::SubTaskAttribution(String sub_task_name,
                                       String script_url,
                                       double start_time,
                                       double duration)
    : sub_task_name_(std::move(sub_task_name)),
      script_url_(std::move(script_url)),
      start_time_(start_time),
      duration_(duration) {}

}  // namespace blink
