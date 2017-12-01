// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_RESOURCE_COORDINATOR_MEMORY_INSTRUMENTATION_TRACE_INSERTION_HELPER_H
#define SERVICES_RESOURCE_COORDINATOR_MEMORY_INSTRUMENTATION_TRACE_INSERTION_HELPER_H

#include "base/macros.h"
#include "base/trace_event/memory_dump_manager.h"
#include "base/trace_event/trace_event.h"
#include "services/resource_coordinator/public/cpp/resource_coordinator_export.h"
#include "services/resource_coordinator/public/interfaces/memory_instrumentation/memory_instrumentation.mojom.h"

namespace memory_instrumentation {

// Observes TraceLog for Enable/Disable events and provides methods for adding dumps to a trace.
class TracingInsertionHelper
    : public base::trace_event::TraceLog::EnabledStateObserver {
 public:
  TracingInsertionHelper(base::trace_event::TraceLog*);
  ~TracingInsertionHelper() override;

  // TraceLog::EnabledStateObserver implementation.
  void OnTraceLogEnabled() override;
  void OnTraceLogDisabled() override;

  bool AddChromeDumpToTraceIfEnabled(
      const base::trace_event::MemoryDumpRequestArgs&,
      const base::ProcessId pid,
      const base::trace_event::ProcessMemoryDump*);
  bool AddOsDumpToTraceIfEnabled(
      const base::trace_event::MemoryDumpRequestArgs&,
      const base::ProcessId,
      const mojom::OSMemDump*,
      const std::vector<mojom::VmRegionPtr>*);

 private:
  // Returns true if the dump mode is allowed for current tracing session.
  bool IsDumpModeAllowed(base::trace_event::MemoryDumpLevelOfDetail) const;

  bool ShouldAddToTrace(const base::trace_event::MemoryDumpRequestArgs&);
  void AddToTrace(const base::trace_event::MemoryDumpRequestArgs&,
                  const base::ProcessId,
                  std::unique_ptr<base::trace_event::TracedValue>);

  base::trace_event::TraceLog* const trace_log_;
  std::unique_ptr<base::trace_event::TraceConfig::MemoryDumpConfig>
      memory_dump_config_;

  DISALLOW_COPY_AND_ASSIGN(TracingInsertionHelper);
};

}  // namespace memory_instrumentation

#endif  // SERVICES_RESOURCE_COORDINATOR_MEMORY_INSTRUMENTATION_TRACE_INSERTION_HELPER_H
