// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/scheduler/util/tracing_helper.h"

#include "base/format_macros.h"
#include "base/strings/stringprintf.h"

namespace blink {
namespace scheduler {

const char kTracingCategoryNameDefault[] = "renderer.scheduler";
const char kTracingCategoryNameInfo[] =
    TRACE_DISABLED_BY_DEFAULT("renderer.scheduler");
const char kTracingCategoryNameDebug[] =
    TRACE_DISABLED_BY_DEFAULT("renderer.scheduler.debug");

namespace {

// No trace events should be created with this category.
const char kTracingCategoryNameVerboseSnapshots[] =
    TRACE_DISABLED_BY_DEFAULT("renderer.scheduler.enable_verbose_snapshots");

}  // namespace

namespace internal {

void ValidateTracingCategory(const char* category) {
  // Category must be a constant defined in tracing_helper.
  // Unfortunately, static_assert won't work with templates because
  // inequality (!=) of linker symbols is undefined in compile-time.
  DCHECK(category == kTracingCategoryNameDefault ||
         category == kTracingCategoryNameInfo ||
         category == kTracingCategoryNameDebug);
}

}  // namespace internal

bool AreVerboseSnapshotsEnabled() {
  bool result = false;
  TRACE_EVENT_CATEGORY_GROUP_ENABLED(kTracingCategoryNameVerboseSnapshots,
                                     &result);
  return result;
}

void WarmupTracingCategories() {
  TRACE_EVENT_WARMUP_CATEGORY(kTracingCategoryNameDefault);
  TRACE_EVENT_WARMUP_CATEGORY(kTracingCategoryNameInfo);
  TRACE_EVENT_WARMUP_CATEGORY(kTracingCategoryNameDebug);
  TRACE_EVENT_WARMUP_CATEGORY(kTracingCategoryNameVerboseSnapshots);
}

std::string PointerToString(const void* pointer) {
  return base::StringPrintf(
      "0x%" PRIx64,
      static_cast<uint64_t>(reinterpret_cast<uintptr_t>(pointer)));
}

double TimeDeltaToMilliseconds(const base::TimeDelta& value) {
  return value.InMillisecondsF();
}

const char* TaskTypeToString(TaskType task_type) {
  switch (task_type) {
    case TaskType::kDOMManipulation:
      return "DOM Manipultion";
    case TaskType::kUserInteraction:
      return "User Interaction";
    case TaskType::kNetworking:
      return "Networking";
    case TaskType::kNetworkingControl:
      return "Networking Control";
    case TaskType::kHistoryTraversal:
      return "History Traversal";
    case TaskType::kEmbed:
      return "Embed";
    case TaskType::kMediaElementEvent:
      return "MediaElement Event";
    case TaskType::kCanvasBlobSerialization:
      return "Canvas Blob Serialization";
    case TaskType::kMicrotask:
      return "Microtask";
    case TaskType::kJavascriptTimer:
      return "Javascript Timer";
    case TaskType::kRemoteEvent:
      return "Remote Event";
    case TaskType::kWebSocket:
      return "WebSocket";
    case TaskType::kPostedMessage:
      return "Posted Message";
    case TaskType::kUnshippedPortMessage:
      return "Unshiped Port Message";
    case TaskType::kFileReading:
      return "File Reading";
    case TaskType::kDatabaseAccess:
      return "Database Access";
    case TaskType::kPresentation:
      return "Presentation";
    case TaskType::kSensor:
      return "Sensor";
    case TaskType::kPerformanceTimeline:
      return "Performance Timeline";
    case TaskType::kWebGL:
      return "WebGL";
    case TaskType::kIdleTask:
      return "Idle Task";
    case TaskType::kMiscPlatformAPI:
      return "Misc Platform API";
    case TaskType::kUnspecedTimer:
      return "Unspeced Timer";
    case TaskType::kUnspecedLoading:
      return "Unspeced Loading";
    case TaskType::kUnthrottled:
      return "Unthrottled";
    case TaskType::kInternalTest:
      return "Internal Test";
    case TaskType::kCount:
      return "Count";
  }
  NOTREACHED();
  return "";
}

const char* OptionalTaskTypeToString(base::Optional<TaskType> opt_task_type) {
  if (!opt_task_type.has_value())
    return nullptr;
  return TaskTypeToString(opt_task_type.value());
}

}  // namespace scheduler
}  // namespace blink
