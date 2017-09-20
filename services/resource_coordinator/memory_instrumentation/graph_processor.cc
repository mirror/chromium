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
    return DumpNode::Entry::Units::kBytes;
  } else if (units == MemoryAllocatorDump::kUnitsObjects) {
    return DumpNode::Entry::Units::kObjects;
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
                           ContainerDump& process_container_dump) {
  for (const auto& path_to_dump : source.allocator_dumps()) {
    const std::string& path = path_to_dump.first;
    const MemoryAllocatorDump& dump = *path_to_dump.second;

    bool is_global = base::StartsWith(path, "global/", CompareCase::SENSITIVE);
    ContainerDump& container =
        is_global ? *global_dump.global_container() : process_container_dump;

    DumpNode* node;
    auto node_iterator = global_dump.nodes_by_guid().find(dump.guid());
    if (node_iterator == global_dump.nodes_by_guid().end()) {
      node = container.CreateNodeInGraph(path);
      global_dump.InsertNodeToGuidMap(dump.guid(), node);
    } else {
      node = node_iterator->second;

      // Check that name of the new dump matches the existing dump.
      DumpNode* existing_node = container.FindNodeInGraph(path);

      if (node != existing_node)
        continue;  // TODO(lalitm) - add a log message for this.

      // Check that the dump is global or the container is the current one.
      if (!is_global && node->container_dump() != &process_container_dump)
        continue;  // TODO(lalitm) - add a log message for this.
    }

    // Copy any entries not already present into the node.
    const std::map<std::string, DumpNode::Entry>& entries = node->entries();
    for (auto& entry : dump.entries()) {
      if (entries.find(entry.name) == entries.end()) {
        AddEntryToNode(*node, entry);
      } else {
        // TODO(lalitm) - add a log message for this.
      }
    }
  }
}

void AddEdges(const base::trace_event::ProcessMemoryDump& source,
              GlobalDump& global_dump) {
  for (auto& guid_to_edge : source.allocator_dumps_edges_for_testing()) {
    auto& edge = guid_to_edge.second;
    auto& nodes_by_guid = global_dump.nodes_by_guid();

    // Find the source and target nodes in the global map by guid.
    auto source = nodes_by_guid.find(edge.source);
    auto target = nodes_by_guid.find(edge.target);
    if (source == nodes_by_guid.end() || target == nodes_by_guid.end()) {
      continue;  // TODO add a log message here.
    }

    // Unlike the JS code, we can assume only ownership edges exist.
    global_dump.AddNodeOwnershipEdge(source->second, target->second,
                                     edge.importance);
  }
}

}  // namespace

std::unique_ptr<GlobalDump> ProcessGlobalDump(
    std::map<ProcessId, ProcessMemoryDump> process_dumps) {
  auto global_dump = std::make_unique<GlobalDump>();

  // First pass: collect all the dumps into trees for processes.
  for (auto& id_to_dump : process_dumps) {
    auto* container_dump =
        global_dump->CreateContainerForProcess(id_to_dump.first);

    // Collects the allocator dumps into a graph and populates the graph
    // with entries.
    CollectAllocatorDumps(id_to_dump.second, *global_dump, *container_dump);
  }

  // Second pass: generate the graph of edges between them.
  for (auto& id_to_dump : process_dumps) {
    // Add edges to the nodes.
    AddEdges(id_to_dump.second, *global_dump);
  }

  return global_dump;
}

}  // namespace memory_instrumentation