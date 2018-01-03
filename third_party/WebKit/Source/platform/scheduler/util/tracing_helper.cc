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
      return "DOMManipultion";
    case TaskType::kUserInteraction:
      return "UserInteraction";
    case TaskType::kNetworking:
      return "Networking";
    case TaskType::kNetworkingControl:
      return "NetworkingControl";
    case TaskType::kHistoryTraversal:
      return "HistoryTraversal";
    case TaskType::kEmbed:
      return "Embed";
    case TaskType::kMediaElementEvent:
      return "MediaElementEvent";
    case TaskType::kCanvasBlobSerialization:
      return "CanvasBlobSerialization";
    case TaskType::kMicrotask:
      return "Microtask";
    case TaskType::kJavascriptTimer:
      return "JavascriptTimer";
    case TaskType::kRemoteEvent:
      return "RemoteEvent";
    case TaskType::kWebSocket:
      return "WebSocket";
    case TaskType::kPostedMessage:
      return "PostedMessage";
    case TaskType::kUnshippedPortMessage:
      return "UnshipedPortMessage";
    case TaskType::kFileReading:
      return "FileReading";
    case TaskType::kDatabaseAccess:
      return "DatabaseAccess";
    case TaskType::kPresentation:
      return "Presentation";
    case TaskType::kSensor:
      return "Sensor";
    case TaskType::kPerformanceTimeline:
      return "PerformanceTimeline";
    case TaskType::kWebGL:
      return "WebGL";
    case TaskType::kIdleTask:
      return "IdleTask";
    case TaskType::kMiscPlatformAPI:
      return "MiscPlatformAPI";
    case TaskType::kUnspecedTimer:
      return "UnspecedTimer";
    case TaskType::kUnspecedLoading:
      return "UnspecedLoading";
    case TaskType::kUnthrottled:
      return "Unthrottled";
    case TaskType::kInternalTest:
      return "InternalTest";
    case TaskType::kCount:
      return "Count";
  }
  NOTREACHED();
  return "";
}

}  // namespace scheduler
}  // namespace blink
