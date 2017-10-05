// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/scheduler/util/renderer_scheduler_tracing.h"

#include "base/logging.h"

namespace blink {
namespace scheduler {

const char* AudioPlayingStateToString(bool is_audio_playing) {
  if (is_audio_playing) {
    return "playing";
  } else {
    return "muted";
  }
}

const char* BackgroundedStateToString(bool is_backgrounded) {
  if (is_backgrounded) {
    return "backgrounded";
  } else {
    return "foregrounded";
  }
}

const char* RAILModeToString(v8::RAILMode rail_mode) {
  switch (rail_mode) {
    case v8::PERFORMANCE_RESPONSE:
      return "response";
    case v8::PERFORMANCE_ANIMATION:
      return "animation";
    case v8::PERFORMANCE_IDLE:
      return "idle";
    case v8::PERFORMANCE_LOAD:
      return "load";
    default:
      NOTREACHED();
      return nullptr;
  }
}

const char* UseCaseToString(UseCase use_case) {
  switch (use_case) {
    case UseCase::NONE:
      return "none";
    case UseCase::COMPOSITOR_GESTURE:
      return "compositor_gesture";
    case UseCase::MAIN_THREAD_CUSTOM_INPUT_HANDLING:
      return "main_thread_custom_input_handling";
    case UseCase::SYNCHRONIZED_GESTURE:
      return "synchronized_gesture";
    case UseCase::TOUCHSTART:
      return "touchstart";
    case UseCase::LOADING:
      return "loading";
    case UseCase::MAIN_THREAD_GESTURE:
      return "main_thread_gesture";
    default:
      NOTREACHED();
      return nullptr;
  }
}

const char* YesNoStateToString(bool state) {
  if (state) {
    return "yes";
  } else {
    return "no";
  }
}

}  // namespace scheduler
}  // namespace blink
