// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/trace_event/sharded_allocation_register.h"

#include <inttypes.h>

#include "base/trace_event/memory_dump_manager.h"
#include "base/trace_event/trace_event.h"
#include "base/trace_event/trace_event_argument.h"
#include "base/trace_event/trace_event_memory_overhead.h"
#include "build/build_config.h"

namespace base {
namespace trace_event {

// This number affects the bucket and capacity counts of AllocationRegister at
// "base/trace_event/heap_profiler_allocation_register.h".
#if defined(OS_ANDROID) || defined(OS_IOS)
const size_t ShardCount = 1;
#elif defined(OS_MACOSX)
const size_t ShardCount = 64;
#else
// Using ShardCount = 64 adds about 1.6GB of committed memory, which triggers
// the sandbox's committed memory limit.
const size_t ShardCount = 16;
#endif

#define TRUNC_SIZE(s) std::min((s), HistogramNumSlots - 1)

ShardedAllocationRegister::ShardedAllocationRegister(const char* allocator_name)
    : enabled_(false) {
  char name[32];
  sprintf(name, "ALLOCATOR %s", allocator_name);
  // Need a long lived string for tracing.
  allocator_name_ = strdup(name);
}

ShardedAllocationRegister::~ShardedAllocationRegister() = default;

void ShardedAllocationRegister::SetEnabled() {
  if (!allocation_registers_)
    allocation_registers_.reset(new RegisterAndLock[ShardCount]);
  if (!histograms_) {
    histograms_.reset(new Histogram[ShardCount]);
    memset(histograms_.get(), 0, sizeof(Histogram) * ShardCount);
  }
  base::subtle::Release_Store(&enabled_, 1);
}

void ShardedAllocationRegister::SetDisabled() {
  base::subtle::Release_Store(&enabled_, 0);
}

bool ShardedAllocationRegister::Insert(const void* address,
                                       size_t size,
                                       const AllocationContext& context) {
  AllocationRegister::AddressHasher hasher;
  size_t index = hasher(address) % ShardCount;
  RegisterAndLock& ral = allocation_registers_[index];
  AutoLock lock(ral.lock);
  histograms_[index][TRUNC_SIZE(size)]++;
  return ral.allocation_register.Insert(address, size, context);
}

void ShardedAllocationRegister::Remove(const void* address) {
  AllocationRegister::AddressHasher hasher;
  size_t index = hasher(address) % ShardCount;
  RegisterAndLock& ral = allocation_registers_[index];
  AutoLock lock(ral.lock);
  return ral.allocation_register.Remove(address);
}

bool ShardedAllocationRegister::Get(
    const void* address,
    AllocationRegister::Allocation* out_allocation) const {
  AllocationRegister::AddressHasher hasher;
  size_t index = hasher(address) % ShardCount;
  RegisterAndLock& ral = allocation_registers_[index];
  AutoLock lock(ral.lock);
  return ral.allocation_register.Get(address, out_allocation);
}

void ShardedAllocationRegister::EstimateTraceMemoryOverhead(
    TraceEventMemoryOverhead* overhead) const {
  size_t allocated = 0;
  size_t resident = 0;
  for (size_t i = 0; i < ShardCount; ++i) {
    RegisterAndLock& ral = allocation_registers_[i];
    AutoLock lock(ral.lock);
    allocated += ral.allocation_register.EstimateAllocatedMemory();
    resident += ral.allocation_register.EstimateResidentMemory();
  }

  overhead->Add(TraceEventMemoryOverhead::kHeapProfilerAllocationRegister,
                allocated, resident);
}

ShardedAllocationRegister::OutputMetrics
ShardedAllocationRegister::UpdateAndReturnsMetrics(MetricsMap& map) const {
  OutputMetrics output_metrics;
  output_metrics.size = 0;
  output_metrics.count = 0;
  Histogram total_hist = {};
  uint64_t total_num_allocs = 0;
  for (size_t i = 0; i < ShardCount; ++i) {
    RegisterAndLock& ral = allocation_registers_[i];
    AutoLock lock(ral.lock);
    for (const auto& alloc_size : ral.allocation_register) {
      AllocationMetrics& metrics = map[alloc_size.context];
      metrics.size += alloc_size.size;
      metrics.count++;

      output_metrics.size += alloc_size.size;
      output_metrics.count++;
      const size_t trunc_sz = TRUNC_SIZE(alloc_size.size);
      const uint64_t num_allocs = histograms_[i][trunc_sz];
      histograms_[i][trunc_sz] = 0;  // Reset for next snapshot
      total_hist[trunc_sz] += num_allocs;
      total_num_allocs += num_allocs;
    }
  }

  ////// compute histogram string
  std::unique_ptr<char[]> hist_str(new char[64 * (HistogramNumSlots + 1)]);
  char* w = &hist_str[0];
  uint64_t cumulative = 0;
  w += sprintf(
      w, "Size                                         #        %%     CDF\n");
  for (size_t sz = 0; sz < HistogramNumSlots; sz++) {
    const uint64_t n = total_hist[sz];
    cumulative += n;
    if (n == 0)
      continue;
    double ratio = 1.0 * n / total_num_allocs;
    char hbar[26];
    memset(hbar, ' ', sizeof(hbar));
    const size_t nbars = static_cast<size_t>(ratio * sizeof(hbar) - 1);
    memset(hbar, '*', nbars);
    hbar[sizeof(hbar) - 1] = '\0';
    w += sprintf(w, "%-4zu: [%s] %12" PRIu64 "  %6.2f %%  %6.2f %%\n", sz, hbar,
                 n, ratio * 100, cumulative * 100.0 / total_num_allocs);
  }
  *w = '\0';
  auto hist_obj = std::make_unique<TracedValue>();
  hist_obj->SetString("h", &hist_str[0]);
  TRACE_COUNTER1(MemoryDumpManager::kTraceCategory, allocator_name_,
                 total_num_allocs);
  TRACE_EVENT_OBJECT_SNAPSHOT_WITH_ID(MemoryDumpManager::kTraceCategory,
                                      allocator_name_, TRACE_ID_LOCAL(this),
                                      std::move(hist_obj));
  ///////////////////////////////
  return output_metrics;
}

ShardedAllocationRegister::RegisterAndLock::RegisterAndLock() = default;
ShardedAllocationRegister::RegisterAndLock::~RegisterAndLock() = default;

}  // namespace trace_event
}  // namespace base
