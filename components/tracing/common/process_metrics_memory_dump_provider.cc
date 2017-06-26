// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/tracing/common/process_metrics_memory_dump_provider.h"

#include "services/resource_coordinator/public/cpp/memory_instrumentation/process_metrics_memory_dump_provider.h"

namespace tracing {

// static
void ProcessMetricsMemoryDumpProvider::RegisterForProcess(
    base::ProcessId process) {
  memory_instrumentation::ProcessMetricsMemoryDumpProvider::RegisterForProcess(
      process);
}

// static
void ProcessMetricsMemoryDumpProvider::UnregisterForProcess(
    base::ProcessId process) {
  memory_instrumentation::ProcessMetricsMemoryDumpProvider::RegisterForProcess(
      process);
}

}  // namespace tracing
