// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/resource_coordinator/memory_instrumentation/graph.h"

#include "base/strings/string_split.h"

namespace memory_instrumentation {

GlobalDump::GlobalDump() {}

GlobalDump::~GlobalDump() {}

ContainerDump* GlobalDump::CreateContainerForProcess(
    base::ProcessId process_id) {
  auto id_to_dump_iterator =
      dumps_.emplace(process_id, std::make_unique<ContainerDump>(this));
  return id_to_dump_iterator.first->second.get();
}

void GlobalDump::InsertNodeToGuidMap(const MemoryAllocatorDumpGuid& guid,
                                     DumpNode* node) {
  nodes_by_guid_.emplace(guid, node);
}

DumpNode* GlobalDump::CreateNode(ContainerDump* container_dump) {
  all_nodes_.push_back(std::make_unique<DumpNode>(container_dump));
  return all_nodes_.rbegin()->get();
}

ContainerDump::ContainerDump(GlobalDump* global_dump)
    : global_dump_(global_dump), root_(global_dump->CreateNode(this)) {}
ContainerDump::~ContainerDump() {}

DumpNode* ContainerDump::CreateNodeInTree(const std::string& path) {
  auto pieces = base::SplitString(path, "/", base::KEEP_WHITESPACE,
                                  base::SPLIT_WANT_NONEMPTY);
  DumpNode* current = root_;
  for (std::string& key : pieces) {
    DumpNode* parent = current;
    current = current->GetChild(key);
    if (!current) {
      current = global_dump_->CreateNode(this);
      parent->PushChild(key, current);
    }
  }
  return current;
}

DumpNode* ContainerDump::FindNodeInTree(const std::string& path) {
  auto pieces = base::SplitString(path, "/", base::KEEP_WHITESPACE,
                                  base::SPLIT_WANT_NONEMPTY);
  DumpNode* current = root_;
  for (std::string& key : pieces) {
    current = current->GetChild(key);
    if (!current)
      return nullptr;
  }
  return current;
}

DumpNode::DumpNode(ContainerDump* container_dump)
    : container_dump_(container_dump) {}
DumpNode::~DumpNode() {}

DumpNode* DumpNode::GetChild(std::string subpath) {
  DCHECK_NE(subpath, "");
  auto child = children_.find(subpath);
  return child == children_.end() ? nullptr : child->second;
}

void DumpNode::PushChild(std::string subpath, DumpNode* node) {
  DCHECK_NE(subpath, "");
  children_.emplace(subpath, node);
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
    : type(DumpNode::Entry::Type::kString),
      units(units),
      value_string(value),
      value_uint64(0) {}

}  // namespace memory_instrumentation