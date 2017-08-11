// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SubTaskAttribution_h
#define SubTaskAttribution_h

#include "platform/heap/Handle.h"
#include "platform/wtf/text/WTFString.h"
namespace blink {

class SubTaskAttribution final
    : public GarbageCollectedFinalized<SubTaskAttribution> {
 public:
  static SubTaskAttribution* Create(String sub_task_name,
                                    String script_url,
                                    double start_time,
                                    double duration) {
    return new SubTaskAttribution(sub_task_name, script_url, start_time,
                                  duration);
  }
  String subTaskName() const;
  String scriptURL() const;
  double startTime() const;
  double duration() const;

  DEFINE_INLINE_VIRTUAL_TRACE() {}

 private:
  SubTaskAttribution(String sub_task_name,
                     String script_url,
                     double start_time,
                     double duration);
  String sub_task_name_;
  String script_url_;
  double start_time_;
  double duration_;
};

}  // namespace blink

#endif  // SubTaskAttribution_h
