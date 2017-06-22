// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/shared_memory_tracker.h"

#include "base/memory/shared_memory.h"
#include "base/strings/string_number_conversions.h"
#include "base/trace_event/memory_allocator_dump_guid.h"
#include "base/trace_event/memory_dump_manager.h"
#include "base/trace_event/process_memory_dump.h"

namespace base {

// static
SharedMemoryTracker* SharedMemoryTracker::GetInstance() {
  static SharedMemoryTracker* instance = new SharedMemoryTracker;
  return instance;
}

// static
std::string SharedMemoryTracker::GetDumpNameForTracing(
    const UnguessableToken& id) {
  return "shared_memory/" + id.ToString();
}

// static
trace_event::MemoryAllocatorDumpGuid
SharedMemoryTracker::GetGlobalDumpGUIDForTracing(const UnguessableToken& id) {
  std::string dump_name = GetDumpNameForTracing(id);
  return trace_event::MemoryAllocatorDumpGuid(dump_name);
}

void SharedMemoryTracker::IncrementMemoryUsage(
    const SharedMemory& shared_memory) {
  AutoLock hold(usages_lock_);
  DCHECK(usages_.find(&shared_memory) == usages_.end());
  usages_[&shared_memory] = shared_memory.mapped_size();
}

void SharedMemoryTracker::DecrementMemoryUsage(
    const SharedMemory& shared_memory) {
  AutoLock hold(usages_lock_);
  DCHECK(usages_.find(&shared_memory) != usages_.end());
  usages_.erase(&shared_memory);
}

bool SharedMemoryTracker::OnMemoryDump(const trace_event::MemoryDumpArgs& args,
                                       trace_event::ProcessMemoryDump* pmd) {
  std::vector<std::tuple<UnguessableToken, uintptr_t, size_t>> usages;
  {
    AutoLock hold(usages_lock_);
    usages.reserve(usages_.size());
    for (const auto& usage : usages_) {
      usages.emplace_back(usage.first->handle().GetGUID(),
                          reinterpret_cast<uintptr_t>(usage.first->memory()),
                          usage.second);
    }
  }
  for (const auto& usage : usages) {
    const UnguessableToken& memory_guid = std::get<0>(usage);
    uintptr_t address = std::get<1>(usage);
    size_t size = std::get<2>(usage);
    std::string dump_name;
    if (memory_guid.is_empty()) {
      // TODO(hajimehoshi): As passing ID across mojo is not implemented yet
      // (crbug/713763), ID can be empty. For such case, use an address instead
      // of GUID so that approximate memory usages are available.
      dump_name = "shared_memory/" + Uint64ToString(address);
    } else {
      dump_name = GetDumpNameForTracing(memory_guid);
    }
    // The dump might already be created at 1) ProcessMemoryDump::
    // CreateSharedMemoryOwnershipEdge without setting its size or 2) single
    // process mode.
    //
    // In 1) case, the dump is created but doesn't have its size. Let's add the
    // size here. In 2) case, the size doesn't need to be added but it is not
    // harmful to override the size scalar with the same size.
    //
    // In both cases, if and only if the dump already exists, an edge between
    // the dump and its global dump exists. Otherwise, an edge needs to be
    // created as importance-overridable. The importance value will be
    // overridden by ProcessMemoryDump later.
    bool need_edge = false;
    trace_event::MemoryAllocatorDump* local_dump =
        pmd->GetAllocatorDump(dump_name);
    if (!local_dump) {
      local_dump = pmd->CreateAllocatorDump(dump_name);
      need_edge = true;
    }
    // TODO(hajimehoshi): The size is not resident size but virtual size so far.
    // Fix this to record resident size.
    local_dump->AddScalar(trace_event::MemoryAllocatorDump::kNameSize,
                          trace_event::MemoryAllocatorDump::kUnitsBytes, size);
    auto global_dump_guid = GetGlobalDumpGUIDForTracing(memory_guid);
    trace_event::MemoryAllocatorDump* global_dump =
        pmd->CreateSharedGlobalAllocatorDump(global_dump_guid);
    global_dump->AddScalar(trace_event::MemoryAllocatorDump::kNameSize,
                           trace_event::MemoryAllocatorDump::kUnitsBytes, size);

    // The edges will be overriden by the clients with correct importance.
    if (need_edge) {
      pmd->AddOverridableOwnershipEdge(local_dump->guid(), global_dump->guid(),
                                       0 /* importance */);
    }
  }
  return true;
}

SharedMemoryTracker::SharedMemoryTracker() {
  trace_event::MemoryDumpManager::GetInstance()->RegisterDumpProvider(
      this, "SharedMemoryTracker", nullptr);
}

SharedMemoryTracker::~SharedMemoryTracker() = default;

}  // namespace
