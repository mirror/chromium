// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/resource_coordinator/memory_instrumentation/graph.h"

#include "base/strings/string_split.h"

namespace memory_instrumentation {

using base::trace_event::MemoryAllocatorDumpGuid;
using DumpGraph = memory_instrumentation::GlobalDumpGraph::DumpGraph;

GlobalDumpGraph::GlobalDumpGraph() {}

GlobalDumpGraph::~GlobalDumpGraph() {}

DumpGraph* GlobalDumpGraph::CreateGraphForProcess(base::ProcessId process_id) {
  auto id_to_dump_iterator = process_dump_graphs_.emplace(
      process_id, std::make_unique<DumpGraph>(this));
  return id_to_dump_iterator.first->second.get();
}

DumpNode* GlobalDumpGraph::CreateNode(DumpGraph* graph) {
  all_nodes_.push_back(std::make_unique<DumpNode>(graph));
  return all_nodes_.rbegin()->get();
}

DumpGraph::DumpGraph(GlobalDumpGraph* global_graph)
    : global_graph_(global_graph), root_(global_graph->CreateNode(this)) {}
DumpGraph::~DumpGraph() {}

DumpNode* DumpGraph::CreateNodeInGraph(const MemoryAllocatorDumpGuid& guid,
                                       const base::StringPiece& path) {
  auto pieces = base::SplitString(path, "/", base::KEEP_WHITESPACE,
                                  base::SPLIT_WANT_NONEMPTY);

  // Perform a tree traversal, creating the nodes if they do not
  // already exist on the path to the child.
  DumpNode* current = root_;
  for (const base::StringPiece& key : pieces) {
    DumpNode* parent = current;
    current = current->GetChild(key);
    if (!current) {
      current = global_graph_->CreateNode(this);
      parent->InsertChild(key, current);
    }
  }

  // Add to the global guid map as well.
  global_graph_->nodes_by_guid_.emplace(guid, current);
  return current;
}

DumpNode* DumpGraph::FindNodeInGraph(const base::StringPiece& path) {
  auto pieces = base::SplitString(path, "/", base::KEEP_WHITESPACE,
                                  base::SPLIT_WANT_NONEMPTY);
  DumpNode* current = root_;
  for (const base::StringPiece& key : pieces) {
    current = current->GetChild(key);
    if (!current)
      return nullptr;
  }
  return current;
}

DumpNode::DumpNode(DumpGraph* dump_graph) : dump_graph_(dump_graph) {}
DumpNode::~DumpNode() {}

DumpNode* DumpNode::GetChild(const base::StringPiece& subpath) {
  DCHECK_NE(subpath, "");
  DCHECK(subpath.find('/') == std::string::npos);

  auto child = children_.find(subpath.as_string());
  return child == children_.end() ? nullptr : child->second;
}

void DumpNode::InsertChild(const base::StringPiece& subpath, DumpNode* node) {
  DCHECK_NE(subpath, "");
  children_.emplace(subpath, node);
}

void DumpNode::AddEntry(std::string name,
                        DumpNode::Entry::ScalarUnits units,
                        uint64_t value) {
  entries_.emplace(name, DumpNode::Entry(units, value));
}

void DumpNode::AddEntry(std::string name,
                        DumpNode::Entry::ScalarUnits units,
                        std::string value) {
  entries_.emplace(name, DumpNode::Entry(units, value));
}

DumpNode::Entry::Entry(Entry::ScalarUnits units, uint64_t value)
    : type(DumpNode::Entry::Type::kUInt64), units(units), value_uint64(value) {}

DumpNode::Entry::Entry(Entry::ScalarUnits units, std::string value)
    : type(DumpNode::Entry::Type::kString),
      units(units),
      value_string(value),
      value_uint64(0) {}

}  // namespace memory_instrumentation