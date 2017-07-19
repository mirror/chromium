// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_METRICS_PROCESS_MEMORY_METRICS_EMITTER_H_
#define CHROME_BROWSER_METRICS_PROCESS_MEMORY_METRICS_EMITTER_H_

#include "base/memory/ref_counted.h"
#include "services/resource_coordinator/public/interfaces/coordination_unit_introspector.mojom.h"
#include "services/resource_coordinator/public/interfaces/memory_instrumentation/memory_instrumentation.mojom.h"
#include "url/gurl.h"

namespace ukm {
class UkmEntryBuilder;
}

// This class asynchronously fetches memory metrics for each process, and then
// emits UMA metrics from those metrics.
// Each instance is self-owned, and will delete itself once it has finished
// emitting metrics.
// This class is an analog to MetricsMemoryDetails, but uses the resource
// coordinator service to fetch data, rather than doing all the processing
// manually.
class ProcessMemoryMetricsEmitter
    : public base::RefCountedThreadSafe<ProcessMemoryMetricsEmitter> {
 public:
  ProcessMemoryMetricsEmitter();

  // This must be called on the main thread of the browser process.
  void FetchAndEmitProcessMemoryMetrics();

 protected:
  virtual ~ProcessMemoryMetricsEmitter();

  // Virtual for testing.
  virtual void ReceivedMemoryDump(
      bool success,
      uint64_t dump_guid,
      memory_instrumentation::mojom::GlobalMemoryDumpPtr ptr);

 private:
  friend class base::RefCountedThreadSafe<ProcessMemoryMetricsEmitter>;

  virtual void ReceivedProcessInfos(
      std::vector<resource_coordinator::mojom::ProcessInfoPtr> process_infos);

  std::unique_ptr<ukm::UkmEntryBuilder> CreateUkmBuilder(const char* event_name,
                                                         const GURL& url);

  memory_instrumentation::mojom::CoordinatorPtr coordinator_;
  resource_coordinator::mojom::CoordinationUnitIntrospectorPtr introspector_;

  // This class sends two asynchronous service requests, whose results need to
  // be collated.
  void CollateResults();
  bool memory_dump_in_process_ = false;
  memory_instrumentation::mojom::GlobalMemoryDumpPtr global_dump_;
  bool get_process_urls_in_progress_ = false;
  std::vector<resource_coordinator::mojom::ProcessInfoPtr> process_infos_;

  DISALLOW_COPY_AND_ASSIGN(ProcessMemoryMetricsEmitter);
};

#endif  // CHROME_BROWSER_METRICS_PROCESS_MEMORY_METRICS_EMITTER_H_
