// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_RENDERER_TRACING_H_
#define THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_RENDERER_TRACING_H_

#include <memory>

#include "platform/scheduler/util/tracing_helper.h"

namespace blink {
namespace scheduler {

const char* AudioPlayingToString(bool is_audio_playing);

class RendererSchedulerTracer {
 public:
  RendererSchedulerTracer()
      : audio_playing_tracer_(
            new StateTracer<bool, kTracingCategoryNameDefault>(
                "RendererScheduler.AudioPlaying",
                this, AudioPlayingToString)) {}

  ObservingTracer<bool>* audio_playing() {
    return audio_playing_tracer_.get();
  }

  void OnTraceLogEnabled() {
    audio_playing_tracer_->OnUpdated();
  }

 private:
  const std::unique_ptr<ObservingTracer<bool>> audio_playing_tracer_;
};

}  // namespace scheduler
}  // namespace blink

#endif  // THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_RENDERER_TRACING_H_
