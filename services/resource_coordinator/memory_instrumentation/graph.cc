// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/resource_coordinator/memory_instrumentation/graph.h"

namespace memory_instrumentation {

GlobalDump::GlobalDump() {}

GlobalDump::~GlobalDump() {}

ProcessDump* GlobalDump::CreateProcessDump(base::ProcessId process_id) {
  auto* dump = new ProcessDump(CreateNode(nullptr));
  dumps[process_id] = std::unique_ptr<ProcessDump>(dump);
  return dump;
}

DumpNode* GlobalDump::CreateNode(DumpNode* parent) {
  auto* node = new DumpNode(this, parent);
  all_nodes.push_back(std::unique_ptr<DumpNode>(node));
  return node;
}

DumpNode::DumpNode(GlobalDump* global, DumpNode* parent) : global_(global) {}

DumpNode::~DumpNode() {}

DumpNode* DumpNode::GetChild(std::string subpath) {
  return children_[subpath];
}

DumpNode* DumpNode::GetOrCreateChild(std::string subpath) {
  auto* child = GetChild(subpath);
  if (child) {
    return child;
  }
  auto* new_node = global_->CreateNode(this);
  children_[subpath] = new_node;
  return new_node;
}

void DumpNode::AddEntry(std::string name,
                        DumpNode::Entry::Units units,
                        uint64_t value) {
  entries_.emplace(name, DumpNode::Entry(units, value));
}

void DumpNode::AddEntry(std::string name,
                        DumpNode::Entry::Units units,
                        std::string value) {
  entries_.emplace(name, DumpNode::Entry(units, value));
}

DumpNode::Entry::Entry(Entry::Units units, uint64_t value)
    : type(DumpNode::Entry::Type::kUInt64), units(units), value_uint64(value) {}

DumpNode::Entry::Entry(Entry::Units units, std::string value)
    : type(DumpNode::Entry::Type::kString), units(units), value_string(value) {}

}  // namespace memory_instrumentation