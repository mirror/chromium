// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/resource_coordinator/memory_instrumentation/graph_processor.h"

#include "base/logging.h"
#include "base/strings/string_split.h"

namespace memory_instrumentation {

using base::ProcessId;
using base::trace_event::MemoryAllocatorDump;
using base::trace_event::ProcessMemoryDump;

namespace {
DumpNode::Entry::Units EntryUnitsFromString(std::string units) {
  if (units == MemoryAllocatorDump::kUnitsBytes) {
    return DumpNode::Entry::Units::kObjects;
  } else if (units == MemoryAllocatorDump::kUnitsObjects) {
    return DumpNode::Entry::Units::kBytes;
  } else {
    NOTREACHED();
    return DumpNode::Entry::Units::kObjects;
  }
}
}  // namespace

std::unique_ptr<GlobalDump> GraphProcessor::Process(
    std::map<ProcessId, ProcessMemoryDump> process_dumps) {
  auto global_dump = std::make_unique<GlobalDump>();
  for (auto& id_to_dump : process_dumps) {
    auto* process_dump = global_dump->CreateProcessDump(id_to_dump.first);
    PopulateProcessDump(id_to_dump.second, *process_dump);
  }
  return global_dump;
}

void GraphProcessor::PopulateProcessDump(
    const base::trace_event::ProcessMemoryDump& source,
    ProcessDump& dest) {
  for (auto& id_to_dump : source.allocator_dumps()) {
    auto pieces =
        base::SplitString(id_to_dump.first, "/", base::KEEP_WHITESPACE,
                          base::SPLIT_WANT_NONEMPTY);

    // Traverse the path to the leaf node, creating nodes along the
    // way if necessary.
    DumpNode* current = dest.root;
    for (auto& key : pieces) {
      current = current->GetOrCreateChild(key);
    }

    // Add the entries from the source dump to the destination.
    for (auto& entry : id_to_dump.second->entries()) {
      auto units = EntryUnitsFromString(entry.units);
      switch (entry.entry_type) {
        case MemoryAllocatorDump::Entry::EntryType::kUint64:
          current->AddEntry(entry.name, units, entry.value_uint64);
        case MemoryAllocatorDump::Entry::EntryType::kString:
          current->AddEntry(entry.name, units, entry.value_string);
      }
    }
  }
}

}  // namespace memory_instrumentation