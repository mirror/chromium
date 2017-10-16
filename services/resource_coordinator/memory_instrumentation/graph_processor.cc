// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/resource_coordinator/memory_instrumentation/graph_processor.h"

#include "base/bind.h"
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

void VisitInDepthFirstPostOrder(
    Node* node,
    std::set<const Node*>* visited,
    std::set<const Node*>* path,
    const base::RepeatingCallback<void(Node*)>& callback) {
  // If the node has already been visited, don't visit it again.
  if (visited->find(node) != visited->end())
    return;

  // Check that the node has not already been encountered on the current path.
  DCHECK(path->find(node) == path->end());
  path->insert(node);

  // Visit all owners of this node.
  for (auto* edge : *node->owned_by_edges()) {
    VisitInDepthFirstPostOrder(edge->source(), visited, path, callback);
  }

  // Visit all children of this node.
  for (auto name_to_child : *node->children()) {
    VisitInDepthFirstPostOrder(name_to_child.second, visited, path, callback);
  }

  // Visit the current node itself.
  callback.Run(node);
  visited->insert(node);

  // The current node is no longer on the path.
  path->erase(node);
}

base::Optional<uint64_t> GetSizeEntryOfNode(Node* node) {
  auto size_it = node->entries()->find("size");
  if (size_it == node->entries()->end())
    return base::nullopt;

  DCHECK(size_it->second.type == Node::Entry::Type::kUInt64);
  DCHECK(size_it->second.units == Node::Entry::ScalarUnits::kBytes);
  return base::Optional<uint64_t>(size_it->second.value_uint64);
}

bool IsNodeDescendentOf(Node* node, Node* possible_parent) {
  const Node* current = node;
  while (current != nullptr) {
    if (current == possible_parent)
      return true;
    current = current->parent();
  }
  return false;
}

base::Optional<uint64_t> AggregateSizeForDescendantNode(Node* root,
                                                        Node* descendant) {
  Edge* owns_edge = descendant->owns_edge();
  if (owns_edge && IsNodeDescendentOf(owns_edge->target(), root))
    return 0;

  if (descendant->children()->size() == 0)
    return GetSizeEntryOfNode(descendant).value_or(0ul);

  base::Optional<uint64_t> size;
  for (auto path_to_child : *descendant->children()) {
    auto c_size = AggregateSizeForDescendantNode(root, path_to_child.second);
    if (size)
      *size += c_size.value_or(0);
    else
      size = std::move(c_size);
  }
  return size;
}

}  // namespace

std::unique_ptr<GlobalDumpGraph> GraphProcessor::ComputeMemoryGraph(
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

  auto* global_root = global_graph->shared_memory_graph()->root();

  // Third pass: mark recursively nodes as weak if they don't have an associated
  // dump and all their children are weak.
  MarkImplicitWeakParentsRecursively(global_root);
  for (auto& pid_to_process : global_graph->process_dump_graphs()) {
    MarkImplicitWeakParentsRecursively(pid_to_process.second->root());
  }

  // Fourth pass: recursively mark nodes as weak if they own a node which is
  // weak or if they have a parent who is weak.
  {
    std::set<const Node*> visited;
    MarkWeakOwnersAndChildrenRecursively(global_root, &visited);
    for (auto& pid_to_process : global_graph->process_dump_graphs()) {
      MarkWeakOwnersAndChildrenRecursively(pid_to_process.second->root(),
                                           &visited);
    }
  }

  // Fifth pass: remove all nodes which are weak (including their descendants)
  // and clean owned by edges to match.
  RemoveWeakNodesRecursively(global_root);
  for (auto& pid_to_process : global_graph->process_dump_graphs()) {
    RemoveWeakNodesRecursively(pid_to_process.second->root());
  }

  // Sixth pass: account for tracing overhead in system memory allocators.
  for (auto& pid_to_process : global_graph->process_dump_graphs()) {
    AssignTracingOverhead("malloc", global_graph.get(),
                          pid_to_process.second.get());
    AssignTracingOverhead("winheap", global_graph.get(),
                          pid_to_process.second.get());
  }

  // Seventh pass: aggregate non-size integer entries into parents and propogate
  // string and int entries for shared graph.
  AggregateNumericsRecursively(global_root);
  PropagateNumericsAndDiagnosticsRecursively(global_root);
  for (auto& pid_to_process : global_graph->process_dump_graphs()) {
    AggregateNumericsRecursively(pid_to_process.second->root());
  }

  {
    std::set<const Node*> visited;
    std::set<const Node*> path;
    VisitInDepthFirstPostOrder(global_root, &visited, &path,
                               base::BindRepeating(&CalculateSizeForNode));
    for (auto& pid_to_process : global_graph->process_dump_graphs()) {
      VisitInDepthFirstPostOrder(pid_to_process.second->root(), &visited, &path,
                                 base::BindRepeating(&CalculateSizeForNode));
    }
  }

  return global_graph;
}

void GraphProcessor::CollectAllocatorDumps(
    const base::trace_event::ProcessMemoryDump& source,
    GlobalDumpGraph* global_graph,
    Process* process_graph) {
  // Turn each dump into a node in the graph of dumps in the appropriate
  // process dump or global dump.
  for (const auto& path_to_dump : source.allocator_dumps()) {
    const std::string& path = path_to_dump.first;
    const MemoryAllocatorDump& dump = *path_to_dump.second;

    // All global dumps (i.e. those starting with global/) should be redirected
    // to the shared graph.
    bool is_global = base::StartsWith(path, "global/", CompareCase::SENSITIVE);
    Process* process =
        is_global ? global_graph->shared_memory_graph() : process_graph;

    Node* node;
    auto node_iterator = global_graph->nodes_by_guid().find(dump.guid());
    if (node_iterator == global_graph->nodes_by_guid().end()) {
      // Storing whether the process is weak here will allow for later
      // computations on whether or not the node should be removed.
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
      DCHECK(node->entries()->find(entry.name) == node->entries()->end());
      AddEntryToNode(node, entry);
    }
  }
}

void GraphProcessor::AddEdges(
    const base::trace_event::ProcessMemoryDump& source,
    GlobalDumpGraph* global_graph) {
  const auto& nodes_by_guid = global_graph->nodes_by_guid();
  for (const auto& guid_to_edge : source.allocator_dumps_edges()) {
    auto& edge = guid_to_edge.second;

    // Find the source and target nodes in the global map by guid.
    auto source_it = nodes_by_guid.find(edge.source);
    auto target_it = nodes_by_guid.find(edge.target);

    if (source_it == nodes_by_guid.end()) {
      // If the source is missing then simply pretend the edge never existed
      // leading to the memory being allocated to the target (if it exists).
      continue;
    } else if (target_it == nodes_by_guid.end()) {
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

void GraphProcessor::MarkImplicitWeakParentsRecursively(Node* node) {
  // Ensure that we aren't in a bad state where we have an implicit node
  // which doesn't have any children.
  DCHECK(node->is_explicit() || !node->children()->empty());

  // Check that at this stage, any node which is weak is only so because
  // it was explicitly created as such.
  DCHECK(!node->is_weak() || node->is_explicit());

  // If a node is already weak then all children will be marked weak at a
  // later stage.
  if (node->is_weak())
    return;

  // Recurse into each child and find out if all the children of this node are
  // weak.
  bool all_children_weak = true;
  for (const auto& path_to_child : *node->children()) {
    MarkImplicitWeakParentsRecursively(path_to_child.second);
    all_children_weak = all_children_weak && path_to_child.second->is_weak();
  }

  // If all the children are weak and the parent is only an implicit one then we
  // consider the parent as weak as well and we will later remove it.
  node->set_weak(!node->is_explicit() && all_children_weak);
}

void GraphProcessor::MarkWeakOwnersAndChildrenRecursively(
    Node* node,
    std::set<const Node*>* visited) {
  // If we've already visited this node then nothing to do.
  if (visited->find(node) != visited->end())
    return;

  // If we haven't visited the node which this node owns then wait for that.
  if (node->owns_edge() &&
      visited->find(node->owns_edge()->target()) == visited->end())
    return;

  // If we haven't visited the node's parent then wait for that.
  if (node->parent() && visited->find(node->parent()) == visited->end())
    return;

  // If either the node we own or our parent is weak, then mark this node
  // as weak.
  if ((node->owns_edge() && node->owns_edge()->target()->is_weak()) ||
      (node->parent() && node->parent()->is_weak())) {
    node->set_weak(true);
  }
  visited->insert(node);

  // Recurse into each owner node to mark any other nodes.
  for (auto* owned_by_edge : *node->owned_by_edges()) {
    MarkWeakOwnersAndChildrenRecursively(owned_by_edge->source(), visited);
  }

  // Recurse into each child and find out if all the children of this node are
  // weak.
  for (const auto& path_to_child : *node->children()) {
    MarkWeakOwnersAndChildrenRecursively(path_to_child.second, visited);
  }
}

void GraphProcessor::RemoveWeakNodesRecursively(Node* parent) {
  for (auto path_to_node_it = parent->children()->begin();
       path_to_node_it != parent->children()->end();) {
    Node* node = path_to_node_it->second;
    if (node->is_weak()) {
      path_to_node_it = parent->children()->erase(path_to_node_it);
    } else {
      // Ensure that we aren't in a situation where we're about to
      // keep a node which owns a weak node (which will be/has been
      // removed.
      DCHECK(!node->owns_edge() || !node->owns_edge()->target()->is_weak());

      RemoveWeakNodesRecursively(node);
      {
        auto* owned_by_edges = node->owned_by_edges();
        auto new_end = std::remove_if(
            owned_by_edges->begin(), owned_by_edges->end(),
            [](Edge* edge) { return edge->source()->is_weak(); });
        owned_by_edges->erase(new_end, owned_by_edges->end());
      }
      ++path_to_node_it;
    }
  }
}

void GraphProcessor::AssignTracingOverhead(base::StringPiece allocator,
                                           GlobalDumpGraph* global_graph,
                                           Process* process) {
  // Check that the tracing dump exists and isn't already owning another node.
  Node* tracing_node = process->FindNode("tracing");
  if (!tracing_node || tracing_node->owns_edge())
    return;

  // Check that the allocator with the given name exists.
  Node* allocator_node = process->FindNode(allocator);
  if (!allocator_node)
    return;

  // Create the node under the allocator to which tracing overhead can be
  // assigned.
  std::string child_name =
      allocator.as_string() + "/allocated_objects/tracing_overhead";
  Node* child_node =
      process->CreateNode(base::nullopt, child_name, false /* weak */);

  // Assign the overhead of tracing to the tracing node.
  global_graph->AddNodeOwnershipEdge(tracing_node, child_node,
                                     0 /* importance */);
}

Node::Entry GraphProcessor::AggregateNumericsForNode(Node* node,
                                                     base::StringPiece name) {
  bool first = true;
  Node::Entry::ScalarUnits units = Node::Entry::ScalarUnits::kObjects;
  uint64_t aggregated = 0;
  for (auto& path_to_child : *node->children()) {
    auto* entries = path_to_child.second->entries();
    auto name_to_entry_it = entries->find(name.as_string());
    if (name_to_entry_it != entries->end()) {
      DCHECK(first || units == name_to_entry_it->second.units);
      units = name_to_entry_it->second.units;
      aggregated += name_to_entry_it->second.value_uint64;
      first = false;
    }
  }
  return Node::Entry(units, aggregated);
}

void GraphProcessor::AggregateNumericsRecursively(Node* node) {
  std::set<std::string> numeric_names;

  for (const auto& path_to_child : *node->children()) {
    AggregateNumericsRecursively(path_to_child.second);
    for (const auto& name_to_entry : *path_to_child.second->entries()) {
      const std::string& name = name_to_entry.first;
      if (name_to_entry.second.type == Node::Entry::Type::kUInt64 &&
          name != "size" && name != "effective_size") {
        numeric_names.insert(name);
      }
    }
  }

  for (auto& name : numeric_names) {
    node->entries()->emplace(name, AggregateNumericsForNode(node, name));
  }
}

void GraphProcessor::PropagateNumericsAndDiagnosticsRecursively(Node* node) {
  for (const auto& name_to_entry : *node->entries()) {
    for (auto* edge : *node->owned_by_edges()) {
      edge->source()->entries()->insert(name_to_entry);
    }
  }
  for (const auto& path_to_child : *node->children()) {
    PropagateNumericsAndDiagnosticsRecursively(path_to_child.second);
  }
}

// Assumes that this function has been called on all children and owner nodes.
void GraphProcessor::CalculateSizeForNode(Node* node) {
  // Get the size at the root node if it exists.
  base::Optional<uint64_t> root_size(GetSizeEntryOfNode(node));

  // Aggregate the size of all the child nodes.
  base::Optional<uint64_t> aggregated_size;
  for (auto path_to_child : *node->children()) {
    auto c_size = AggregateSizeForDescendantNode(node, path_to_child.second);
    if (aggregated_size)
      *aggregated_size += c_size.value_or(0ul);
    else
      aggregated_size = std::move(c_size);
  }

  // Check that if both aggregated and root sizes exist that the root size
  // is bigger than the aggregated.
  DCHECK(!root_size || !aggregated_size || *root_size >= *aggregated_size);

  // Calculate the maximal size of an owner node.
  base::Optional<uint64_t> max_owner_size;
  for (auto* edge : *node->owned_by_edges()) {
    auto o_size = GetSizeEntryOfNode(edge->source());
    if (max_owner_size)
      *max_owner_size = std::max(o_size.value_or(0ul), *max_owner_size);
    else
      max_owner_size = std::move(o_size);
  }

  // Check that if both owner and root sizes exist that the root size
  // is bigger than the owner.
  DCHECK(!root_size || !max_owner_size || *root_size >= *max_owner_size);

  // Clear out any existing size entry which may exist.
  node->entries()->erase("size");

  // If no inference about size can be made then simply return.
  if (!root_size && !aggregated_size && !max_owner_size)
    return;

  // Update the node with the new size entry.
  uint64_t aggregated_size_value = aggregated_size.value_or(0ul);
  uint64_t process_size =
      std::max({root_size.value_or(0ul), aggregated_size_value,
                max_owner_size.value_or(0ul)});
  node->entries()->emplace(
      "size", Node::Entry(Node::Entry::ScalarUnits::kBytes, process_size));

  // If this is an intermediate node then add a ghost node which stores
  // all sizes not accounted for by the children.
  uint64_t unaccounted = process_size - aggregated_size_value;
  if (unaccounted > 0 && !node->children()->empty()) {
    Node* unspecified = node->CreateChild("<unspecified>");
    unspecified->entries()->emplace(
        "size", Node::Entry(Node::Entry::ScalarUnits::kBytes, unaccounted));
  }
}

}  // namespace memory_instrumentation