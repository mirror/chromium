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
using Edge = memory_instrumentation::GlobalDumpGraph::Edge;
using Node = memory_instrumentation::GlobalDumpGraph::Node;
using Process = memory_instrumentation::GlobalDumpGraph::Process;

namespace {

Node::Entry::ScalarUnits EntryUnitsFromString(std::string units) {
  if (units == MemoryAllocatorDump::kUnitsBytes) {
    return Node::Entry::ScalarUnits::kBytes;
  } else if (units == MemoryAllocatorDump::kUnitsObjects) {
    return Node::Entry::ScalarUnits::kObjects;
  } else {
    // Invalid units so we just return a value of the correct type.
    return Node::Entry::ScalarUnits::kObjects;
  }
}

void AddEntryToNode(Node* node, const MemoryAllocatorDump::Entry& entry) {
  switch (entry.entry_type) {
    case MemoryAllocatorDump::Entry::EntryType::kUint64:
      node->AddEntry(entry.name, EntryUnitsFromString(entry.units),
                     entry.value_uint64);
      break;
    case MemoryAllocatorDump::Entry::EntryType::kString:
      node->AddEntry(entry.name, entry.value_string);
      break;
  }
}

void CollectAllocatorDumps(const base::trace_event::ProcessMemoryDump& source,
                           GlobalDumpGraph* global_graph,
                           Process* process_graph) {
  for (const auto& path_to_dump : source.allocator_dumps()) {
    const std::string& path = path_to_dump.first;
    const MemoryAllocatorDump& dump = *path_to_dump.second;

    bool is_global = base::StartsWith(path, "global/", CompareCase::SENSITIVE);
    Process* process =
        is_global ? global_graph->shared_memory_graph() : process_graph;

    Node* node;
    auto node_iterator = global_graph->nodes_by_guid().find(dump.guid());
    if (node_iterator == global_graph->nodes_by_guid().end()) {
      bool is_weak = dump.flags() & MemoryAllocatorDump::Flags::WEAK;
      node = process->CreateNode(dump.guid(), path, is_weak);
    } else {
      node = node_iterator->second;

      DCHECK_EQ(node, process->FindNode(path))
          << "Nodes have different paths but same GUIDs";
      DCHECK(is_global) << "Multiple nodes have same GUID without being global";
    }

    // Copy any entries not already present into the node.
    for (auto& entry : dump.entries()) {
      // Check that there are not multiple entries with the same name.
      DCHECK(node->entries().find(entry.name) == node->entries().end());
      AddEntryToNode(node, entry);
    }
  }
}

void AddEdges(const base::trace_event::ProcessMemoryDump& source,
              GlobalDumpGraph* global_graph) {
  auto& nodes_by_guid = global_graph->nodes_by_guid();
  for (auto& guid_to_edge : source.allocator_dumps_edges_for_testing()) {
    auto& edge = guid_to_edge.second;

    // Find the source and target nodes in the global map by guid.
    auto source_it = nodes_by_guid.find(edge.source);
    auto target_it = nodes_by_guid.find(edge.target);

    bool source_lost = source_it == nodes_by_guid.end();
    bool target_lost = target_it == nodes_by_guid.end();
    if (source_lost) {
      // If the source is missing then simply pretend the edge never existed
      // leading to the memory being allocated to the target (if it exists).
      continue;
    } else if (target_lost) {
      // If the target is lost but the source is present, then also ignore
      // this edge for now.
      // TODO(lalitm): see crbug.com/770712 for the permanent fix for this
      // issue.
      continue;
    } else {
      // Add an edge indicating the source node owns the memory of the
      // target node with the given importance of the edge.
      global_graph->AddNodeOwnershipEdge(source_it->second, target_it->second,
                                         edge.importance);
    }
  }
}

void MarkWeakNodesRecursively(Node* node) {
  if (node->is_weak())
    return;

  // While a node is also considered weak if its parent is weak, this is
  // considered implicit in this algorithm. Therefore, we will not mark
  // this node based on its parents weakness.
  if (node->owns_edge() && node->owns_edge()->target()->is_weak()) {
    node->set_weak(true);
    return;
  }

  // Recursively mark nodes as weak then store whether all children are weak.
  bool all_children_weak = true;
  for (auto& path_to_child : *node->children()) {
    MarkWeakNodesRecursively(path_to_child.second);
    all_children_weak = all_children_weak && path_to_child.second->is_weak();
  }

  // We mark a node as weak if there was no explicit dump associated with it and
  // all its children are weak.
  node->set_weak(!node->is_explicit() && all_children_weak);
}

void RemoveWeakNodesRecursively(Node* parent) {
  for (auto path_to_node_it = parent->children()->begin();
       path_to_node_it != parent->children()->end();) {
    Node* node = path_to_node_it->second;
    if (node->is_weak()) {
      path_to_node_it = parent->children()->erase(path_to_node_it);
    } else {
      RemoveWeakNodesRecursively(node);

      auto* owned_by_edges = node->owned_by_edges();
      std::remove_if(owned_by_edges->begin(), owned_by_edges->end(),
                     [](Edge* edge) { return edge->source()->is_weak(); });
      ++path_to_node_it;
    }
  }
}

}  // namespace

std::unique_ptr<GlobalDumpGraph> ComputeMemoryGraph(
    const std::map<ProcessId, ProcessMemoryDump>& process_dumps) {
  auto global_graph = std::make_unique<GlobalDumpGraph>();

  // First pass: collects allocator dumps into a graph and populate
  // with entries.
  for (auto& pid_to_dump : process_dumps) {
    auto* graph = global_graph->CreateGraphForProcess(pid_to_dump.first);
    CollectAllocatorDumps(pid_to_dump.second, global_graph.get(), graph);
  }

  // Second pass: generate the graph of edges between the nodes.
  for (auto& pid_to_dump : process_dumps) {
    AddEdges(pid_to_dump.second, global_graph.get());
  }

  // Third pass: mark recursively nodes as weak if they own a node which is
  // weak or they don't have an associated dump and all theird children are
  // weak. Implicitly consider child nodes as weak if parent is weak.
  MarkWeakNodesRecursively(global_graph->shared_memory_graph()->root());
  for (auto& pid_to_process : global_graph->process_dump_graphs()) {
    MarkWeakNodesRecursively(pid_to_process.second->root());
  }

  // Fourth pass: remove all nodes which are weak (including their descendants)
  // and clean owned by edges to match.
  RemoveWeakNodesRecursively(global_graph->shared_memory_graph()->root());
  for (auto& pid_to_process : global_graph->process_dump_graphs()) {
    RemoveWeakNodesRecursively(pid_to_process.second->root());
  }

  return global_graph;
}

}  // namespace memory_instrumentation