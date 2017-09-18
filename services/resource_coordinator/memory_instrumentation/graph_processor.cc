// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/resource_coordinator/memory_instrumentation/graph_processor.h"

#include "base/logging.h"
#include "base/strings/string_split.h"

namespace memory_instrumentation {

using base::CompareCase;
using base::ProcessId;
using base::trace_event::MemoryAllocatorDump;
using base::trace_event::MemoryAllocatorDumpGuid;
using base::trace_event::ProcessMemoryDump;

namespace {

DumpNode::Entry::Units EntryUnitsFromString(std::string units) {
  if (units == MemoryAllocatorDump::kUnitsBytes) {
    return DumpNode::Entry::Units::kObjects;
  } else if (units == MemoryAllocatorDump::kUnitsObjects) {
    return DumpNode::Entry::Units::kBytes;
  } else {
    LOG(ERROR) << "Invalid units for entry in memory dump";
    return DumpNode::Entry::Units::kObjects;
  }
}

void AddEntryToNode(DumpNode& node, const MemoryAllocatorDump::Entry& entry) {
  auto units = EntryUnitsFromString(entry.units);
  switch (entry.entry_type) {
    case MemoryAllocatorDump::Entry::EntryType::kUint64:
      node.AddEntry(entry.name, units, entry.value_uint64);
      break;
    case MemoryAllocatorDump::Entry::EntryType::kString:
      node.AddEntry(entry.name, units, entry.value_string);
      break;
  }
}

void CollectAllocatorDumps(const base::trace_event::ProcessMemoryDump& source,
                           GlobalDump& global_dump,
                           ContainerDump& container_dump) {
  for (const auto& path_to_dump : source.allocator_dumps()) {
    const std::string& path = path_to_dump.first;
    const MemoryAllocatorDump& dump = *path_to_dump.second;

    DumpNode* node;
    bool is_global = base::StartsWith(path, "global/", CompareCase::SENSITIVE);
    auto node_iterator = global_dump.nodes_by_guid().find(dump.guid());
    if (node_iterator == global_dump.nodes_by_guid().end()) {
      if (is_global)
        node = global_dump.global_container()->CreateNodeInTree(path);
      else
        node = container_dump.CreateNodeInTree(path);

      global_dump.InsertNodeToGuidMap(dump.guid(), node);
    } else {
      node = node_iterator->second;

      // Check that name of the new dump matches the existing dump.
      DumpNode* existing_node;
      if (is_global)
        existing_node = global_dump.global_container()->FindNodeInTree(path);
      else
        existing_node = container_dump.FindNodeInTree(path);

      if (node != existing_node)
        continue;  // TODO - add a log message for this.

      // Check that the dump is global or the container is the current one.
      if (!is_global && node->container_dump() != &container_dump)
        continue;  // TODO - add a log message for this.
    }

    // Copy any entries not already present into the node.
    const std::map<std::string, DumpNode::Entry>& entries = node->entries();
    for (auto& entry : dump.entries()) {
      if (entries.find(entry.name) == entries.end())
        AddEntryToNode(*node, entry);
    }
  }
}

}  // namespace

std::unique_ptr<GlobalDump> ProcessGlobalDump(
    std::map<ProcessId, ProcessMemoryDump> process_dumps) {
  auto global_dump = std::make_unique<GlobalDump>();

  for (auto& id_to_dump : process_dumps) {
    auto* container_dump =
        global_dump->CreateContainerForProcess(id_to_dump.first);

    // Collect the allocator dumps into three maps.
    CollectAllocatorDumps(id_to_dump.second, *global_dump, *container_dump);
  }
  return global_dump;
}

}  // namespace memory_instrumentation