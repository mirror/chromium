// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/tracing/ftrace.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "base/files/file_util.h"
#include "base/strings/string_piece.h"
#include "base/strings/string_util.h"

namespace chromecast {
namespace tracing {

namespace {

static const char kPathTracefs[] = "/sys/kernel/debug/tracing";
static const char kTraceFileTracingOn[] = "tracing_on";
static const char kTraceFileTraceMarker[] = "trace_marker";
static const char kTraceFileSetEvent[] = "set_event";
static const char kTraceFileTraceClock[] = "trace_clock";
static const char kTraceFileTrace[] = "trace";
static const char kTraceFileBufferSizeKb[] = "buffer_size_kb";

static const char kBufferSizeKbRunning[] = "7040";
static const char kBufferSizeKbIdle[] = "1408";

static const char* const kCategories[] = {"gfx", "input", "power", "sched",
                                          "workq"};

// TODO(spang): Load these lists from a configuration file.
static const char* const kGfxEvents[] = {
    "i915:i915_flip_request",        "i915:i915_flip_complete",
    "i915:i915_gem_object_pwrite",   "i915:intel_gpu_freq_change",
    "exynos:exynos_flip_request",    "exynos:exynos_flip_complete",
    "exynos:exynos_page_flip_state", "drm:drm_vblank_event",
};

static const char* const kInputEvents[] = {
    "irq:irq_threaded_handler_entry", "irq:irq_threaded_handler_exit",
};

static const char* const kPowerEvents[] = {
    "power:cpu_idle",
    "power:cpu_frequency",
    "mali:mali_dvfs_set_clock",
    "mali:mali_dvfs_set_voltage",
    "cpufreq_interactive:cpufreq_interactive_boost",
    "cpufreq_interactive:cpufreq_interactive_unboost",
    "exynos_busfreq:exynos_busfreq_target_int",
    "exynos_busfreq:exynos_busfreq_target_mif",
};

static const char* const kSchedEvents[] = {
    "sched:sched_switch", "sched:sched_wakeup",
};

static const char* const kWorkqEvents[] = {
    "workqueue:workqueue_execute_start", "workqueue:workqueue_execute_end",
};

void AddCategoryEvents(const std::string& category,
                       std::vector<std::string>* events) {
  if (category == "gfx") {
    std::copy(kGfxEvents, kGfxEvents + arraysize(kGfxEvents),
              std::back_inserter(*events));
    return;
  }
  if (category == "input") {
    std::copy(kInputEvents, kInputEvents + arraysize(kInputEvents),
              std::back_inserter(*events));
    return;
  }
  if (category == "power") {
    std::copy(kPowerEvents, kPowerEvents + arraysize(kPowerEvents),
              std::back_inserter(*events));
    return;
  }
  if (category == "sched") {
    std::copy(kSchedEvents, kSchedEvents + arraysize(kSchedEvents),
              std::back_inserter(*events));
    return;
  }
  if (category == "workq") {
    std::copy(kWorkqEvents, kWorkqEvents + arraysize(kWorkqEvents),
              std::back_inserter(*events));
    return;
  }
}

bool WriteTracingFile(const char* trace_file, base::StringPiece contents) {
  base::FilePath path = base::FilePath(kPathTracefs).Append(trace_file);

  if (base::WriteFile(path, contents.data(), contents.size()) < 0) {
    PLOG(ERROR) << "write: " << path;
    return false;
  }

  return true;
}

bool AppendTracingFile(const char* trace_file, base::StringPiece contents) {
  base::FilePath path = base::FilePath(kPathTracefs).Append(trace_file);

  if (base::AppendToFile(path, contents.data(), contents.size()) < 0) {
    PLOG(ERROR) << "write: " << path;
    return false;
  }

  return true;
}

}  // namespace

bool IsValidCategory(base::StringPiece str) {
  for (const char* category : kCategories) {
    if (category == str) {
      return true;
    }
  }

  return false;
}

bool StartFtrace(std::vector<std::string> categories) {
  // Disable tracing and clear events.
  if (!WriteTracingFile(kTraceFileTracingOn, "0"))
    return false;
  if (!WriteTracingFile(kTraceFileSetEvent, ""))
    return false;

  // Use CLOCK_MONOTONIC so that kernel timestamps align with std::steady_clock
  // and base::TimeTicks.
  if (!WriteTracingFile(kTraceFileTraceClock, "mono"))
    return false;

  if (categories.size() == 0) {
    LOG(ERROR) << "No categories to enable";
    return false;
  }

  std::vector<std::string> events;
  for (const auto& category : categories)
    AddCategoryEvents(category, &events);

  if (events.size() == 0) {
    LOG(ERROR) << "No events to enable";
    return false;
  }

  for (const auto& event : events) {
    if (!AppendTracingFile(kTraceFileSetEvent, event))
      LOG(WARNING) << "Failed to enable event: " << event;
  }

  if (!WriteTracingFile(kTraceFileBufferSizeKb, kBufferSizeKbRunning))
    return false;
  if (!WriteTracingFile(kTraceFileTracingOn, "1"))
    return false;

  return true;
}

bool WriteFtraceTimeSyncMarker() {
  return WriteTracingFile(kTraceFileTraceMarker,
                          "trace_event_clock_sync: parent_ts=0");
}

bool StopFtrace() {
  if (!WriteTracingFile(kTraceFileTracingOn, "0"))
    return false;
  return true;
}

base::ScopedFD GetFtraceData() {
  base::FilePath path = base::FilePath(kPathTracefs).Append(kTraceFileTrace);
  base::ScopedFD trace_data(HANDLE_EINTR(
      open(path.value().c_str(), O_RDONLY | O_CLOEXEC | O_NONBLOCK)));
  if (!trace_data.is_valid())
    PLOG(ERROR) << "open: " << path.value();
  return trace_data;
}

bool ClearFtrace() {
  if (!WriteTracingFile(kTraceFileBufferSizeKb, kBufferSizeKbIdle))
    return false;
  if (!WriteTracingFile(kTraceFileTrace, "0"))
    return false;
  return true;
}

}  // namespace tracing
}  // namespace chromecast
