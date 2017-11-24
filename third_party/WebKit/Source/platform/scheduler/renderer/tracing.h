// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_RENDERER_TRACING_H_
#define THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_RENDERER_TRACING_H_

#include <memory>

#include "base/macros.h"
#include "platform/scheduler/util/tracing_helper.h"

namespace blink {
namespace scheduler {

class RendererSchedulerTracer {
 public:
  RendererSchedulerTracer();
  ~RendererSchedulerTracer();

  void OnTraceLogEnabled() {
    audio_playing_tracer_->OnUpdated();
  }

  const std::unique_ptr<ObservingTracer<bool>> audio_playing;
  const std::unique_ptr<ObservingTracer<bool>> loading_tasks_seem_expensive;
  const std::unique_ptr<ObservingTracer<bool>> timer_tasks_seem_expensive;

 private:
  DISALLOW_COPY_AND_ASSIGN(RendererSchedulerTracer);
};

}  // namespace scheduler
}  // namespace blink

#endif  // THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_RENDERER_TRACING_H_
