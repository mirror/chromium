// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/timing/SubTaskAttribution.h"

namespace blink {

SubTaskAttribution::SubTaskAttribution(String sub_task_name,
                                       String script_url,
                                       double start_time,
                                       double duration)
    : sub_task_name_(sub_task_name),
      script_url_(script_url),
      start_time_(start_time),
      duration_(duration) {}

String SubTaskAttribution::subTaskName() const {
  return sub_task_name_;
}

String SubTaskAttribution::scriptURL() const {
  return script_url_;
}

double SubTaskAttribution::startTime() const {
  return start_time_;
}

double SubTaskAttribution::duration() const {
  return duration_;
}

void SubTaskAttribution::setHighResStartTime(DOMHighResTimeStamp start_time) {
  start_time_ = start_time;
}

void SubTaskAttribution::setHighResDuration(DOMHighResTimeStamp duration) {
  duration_ = duration;
}

DOMHighResTimeStamp SubTaskAttribution::highResStartTime() const {
  return high_res_start_time_;
}

DOMHighResTimeStamp SubTaskAttribution::highResDuration() const {
  return high_res_duration_;
}
}  // namespace blink
