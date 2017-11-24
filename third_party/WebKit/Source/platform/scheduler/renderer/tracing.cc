// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/scheduler/renderer/tracing.h"

namespace blink {
namespace scheduler {

namespace {

const char* AudioPlayingToString(bool is_audio_playing) {
  if (is_audio_playing) {
    return "playing";
  } else {
    return "muted";
  }
}

}  // namespace


RendererSchedulerTracer::RendererSchedulerTracer()
    : audio_playing_(new StateTracer<bool, kTracingCategoryNameDefault>(
          "RendererScheduler.AudioPlaying", this, AudioPlayingToString)),
      loading_tasks_seem_expensive(new StateTracer<bool, kTracingCategoryNameDefault>(
          "RendererScheduler.LoadingTasksSeemExpensive", this, YesNoToString)) {}

}  // namespace scheduler
}  // namespace blink
