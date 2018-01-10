// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_RESOURCE_COORDINATOR_PUBLIC_CPP_MEMORY_INSTRUMENTATION_GLOBAL_MEMORY_DUMP_H_
#define SERVICES_RESOURCE_COORDINATOR_PUBLIC_CPP_MEMORY_INSTRUMENTATION_GLOBAL_MEMORY_DUMP_H_

#include "base/optional.h"
#include "services/resource_coordinator/public/interfaces/memory_instrumentation/memory_instrumentation.mojom.h"

namespace memory_instrumentation {

// The returned data structure to consumers of the memory_instrumentation
// service containing dumps for each process.
class SERVICES_RESOURCE_COORDINATOR_PUBLIC_CPP_EXPORT GlobalMemoryDump {
 public:
  class ProcessDump {
   public:
    ProcessDump(mojom::ProcessMemoryDumpPtr process_memory_dump);
    ~ProcessDump();

    base::Optional<uint64_t> GetMetricForDump(const std::string& dump_name,
                                              const std::string& metric_name);

    base::ProcessId pid() const { return mojo_dump_->pid; }
    mojom::ProcessType process_type() const { return mojo_dump_->process_type; }

    // TODO(lalitm): remove this when all consumers have migrated to using
    // GetMetricForDump.
    const mojom::ChromeMemDump& chrome_dump() const {
      return *mojo_dump_->chrome_dump;
    }
    const mojom::OSMemDump& os_dump() const { return *mojo_dump_->os_dump; }

   private:
    mojom::ProcessMemoryDumpPtr mojo_dump_;

    DISALLOW_COPY_AND_ASSIGN(ProcessDump);
  };

  ~GlobalMemoryDump();

  // Creates an owned instance of this class wrapping the given mojo struct.
  static std::unique_ptr<GlobalMemoryDump> MoveFrom(
      mojom::GlobalMemoryDumpPtr ptr);

  const std::forward_list<ProcessDump>& process_dumps() const {
    return process_dumps_;
  }

 private:
  GlobalMemoryDump(std::vector<mojom::ProcessMemoryDumpPtr> process_dumps);

  std::forward_list<ProcessDump> process_dumps_;

  DISALLOW_COPY_AND_ASSIGN(GlobalMemoryDump);
};

}  // namespace memory_instrumentation

#endif  // SERVICES_RESOURCE_COORDINATOR_PUBLIC_CPP_MEMORY_INSTRUMENTATION_GLOBAL_MEMORY_DUMP_H_
