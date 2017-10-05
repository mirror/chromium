// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_RENDERER_RENDERER_SCHEDULER_TYPES_H_
#define THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_RENDERER_RENDERER_SCHEDULER_TYPES_H_

namespace blink {
namespace scheduler {

// Keep UseCaseToString in sync with this enum.
enum class UseCase {
  // No active use case detected.
  NONE,
  // A continuous gesture (e.g., scroll, pinch) which is being driven by the
  // compositor thread.
  COMPOSITOR_GESTURE,
  // An unspecified touch gesture which is being handled by the main thread.
  // Note that since we don't have a full view of the use case, we should be
  // careful to prioritize all work equally.
  MAIN_THREAD_CUSTOM_INPUT_HANDLING,
  // A continuous gesture (e.g., scroll, pinch) which is being driven by the
  // compositor thread but also observed by the main thread. An example is
  // synchronized scrolling where a scroll listener on the main thread changes
  // page layout based on the current scroll position.
  SYNCHRONIZED_GESTURE,
  // A gesture has recently started and we are about to run main thread touch
  // listeners to find out the actual gesture type. To minimize touch latency,
  // only input handling work should run in this state.
  TOUCHSTART,
  // A page is loading.
  LOADING,
  // A continuous gesture (e.g., scroll) which is being handled by the main
  // thread.
  MAIN_THREAD_GESTURE,
  // Must be the last entry.
  USE_CASE_COUNT,
  FIRST_USE_CASE = NONE,
};

}  // namespace scheduler
}  // namespace blink

#endif  // THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_RENDERER_RENDERER_SCHEDULER_TYPES_H_
