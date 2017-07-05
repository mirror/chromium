// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef SERVICES_RESOURCE_COORDINATOR_PUBLIC_CPP_MEMORY_INSTRUMENTATION_OS_METRICS_H_
#define SERVICES_RESOURCE_COORDINATOR_PUBLIC_CPP_MEMORY_INSTRUMENTATION_OS_METRICS_H_

#include "base/process/process_handle.h"
#include "base/trace_event/process_memory_dump.h"
#include "build/build_config.h"
#include "services/resource_coordinator/public/cpp/resource_coordinator_export.h"
#include "services/resource_coordinator/public/interfaces/memory_instrumentation/memory_instrumentation.mojom.h"

namespace memory_instrumentation {

namespace OSMetrics {

mojom::RawOSMemDumpPtr SERVICES_RESOURCE_COORDINATOR_PUBLIC_CPP_EXPORT
GetOSMemoryDump(base::ProcessId pid);

bool SERVICES_RESOURCE_COORDINATOR_PUBLIC_CPP_EXPORT
DumpProcessMemoryMaps(base::ProcessId, base::trace_event::ProcessMemoryDump*);

void FillOSMemoryDump(base::ProcessId pid, mojom::RawOSMemDump* dump);

#if defined(OS_LINUX) || defined(OS_ANDROID)
void SERVICES_RESOURCE_COORDINATOR_PUBLIC_CPP_EXPORT
SetProcSmapsForTesting(FILE*);
#endif

}  // namespace OSMetrics

}  // namespace memory_instrumentation

#endif  // SERVICES_RESOURCE_COORDINATOR_PUBLIC_CPP_MEMORY_INSTRUMENTATION_OS_METRICS_H_
