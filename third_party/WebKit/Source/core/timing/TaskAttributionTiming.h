// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TaskAttributionTiming_h
#define TaskAttributionTiming_h

#include "core/timing/PerformanceEntry.h"
#include "platform/heap/Handle.h"
#include "platform/wtf/Forward.h"
#include "platform/wtf/text/WTFString.h"

namespace blink {

class TaskAttributionTiming final : public PerformanceEntry {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static TaskAttributionTiming* Create(String script_url,
                                       String type,
                                       String container_type,
                                       String container_src,
                                       String container_id,
                                       String container_name,
                                       double start_time,
                                       double finish_time) {
    return new TaskAttributionTiming(script_url, type, container_type,
                                     container_src, container_id,
                                     container_name, start_time, finish_time);
  }

  static TaskAttributionTiming* Create(String type,
                                       String container_type,
                                       String container_src,
                                       String container_id,
                                       String container_name) {
    return new TaskAttributionTiming("place-holder", type, container_type,
                                     container_src, container_id,
                                     container_name, 0.0, 0.0);
  }
  String scriptURL() const;
  String containerType() const;
  String containerSrc() const;
  String containerId() const;
  String containerName() const;

  DECLARE_VIRTUAL_TRACE();

  ~TaskAttributionTiming() override;

 private:
  TaskAttributionTiming(String script_url,
                        String type,
                        String container_type,
                        String container_src,
                        String container_id,
                        String container_name,
                        double start_time,
                        double finish_time);
  String script_url_;
  String container_type_;
  String container_src_;
  String container_id_;
  String container_name_;
};

}  // namespace blink

#endif  // TaskAttributionTiming_h
