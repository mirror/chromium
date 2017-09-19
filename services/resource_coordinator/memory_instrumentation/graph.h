// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_RESOURCE_COORDINATOR_MEMORY_INSTRUMENTATION_GRAPH_H_
#define SERVICES_RESOURCE_COORDINATOR_MEMORY_INSTRUMENTATION_GRAPH_H_

#include <map>
#include <memory>
#include <vector>

#include "base/gtest_prod_util.h"
#include "base/process/process_handle.h"
#include "base/trace_event/process_memory_dump.h"

namespace memory_instrumentation {

using base::trace_event::MemoryAllocatorDumpGuid;

class DumpNode;
class ContainerDump;
class DumpEdge;

class GlobalDump {
  friend class ContainerDump;

 public:
  using ContainerDumpMap =
      std::map<base::ProcessId, std::unique_ptr<ContainerDump>>;
  using GuidNodeMap = std::map<MemoryAllocatorDumpGuid, DumpNode*>;

  GlobalDump();
  ~GlobalDump();

  ContainerDump* CreateContainerForProcess(base::ProcessId process_id);
  void InsertNodeToGuidMap(const MemoryAllocatorDumpGuid& guid, DumpNode* node);
  bool AddNodeOwnershipEdge(DumpNode* owner, DumpNode* owned, int priority);

  ContainerDump* global_container() { return global_container_; }
  const GuidNodeMap& nodes_by_guid() { return nodes_by_guid_; }
  const ContainerDumpMap& container_dumps() { return dumps_; }

 private:
  DumpNode* CreateNode(ContainerDump* container_dump);

  std::vector<std::unique_ptr<DumpNode>> all_nodes_;
  std::vector<std::unique_ptr<DumpEdge>> all_edges_;

  GuidNodeMap nodes_by_guid_;
  ContainerDump* global_container_;
  ContainerDumpMap dumps_;
};

class ContainerDump {
 public:
  ContainerDump(GlobalDump* global_dump);
  ~ContainerDump();

  DumpNode* CreateNodeInTree(const std::string& path);
  DumpNode* FindNodeInTree(const std::string& path);
  DumpNode* dump_root() { return root_; }

 private:
  GlobalDump* global_dump_;
  DumpNode* root_;
};

class DumpNode {
 public:
  struct Entry {
    enum Type {
      kUInt64,
      kString,
    };

    enum Units {
      kObjects,
      kBytes,
    };

    Entry(Units units, uint64_t value);
    Entry(Units units, std::string value);

    const Type type;
    const Units units;

    const std::string value_string;
    const std::uint64_t value_uint64;
  };

  DumpNode(ContainerDump* container_dump);
  ~DumpNode();

  DumpNode* GetChild(std::string subpath);
  void PushChild(std::string subpath, DumpNode* node);

  void SetOwnsEdge(DumpEdge* edge);
  void AddOwnedByEdge(DumpEdge* edge);

  void AddEntry(std::string name, Entry::Units units, uint64_t value);
  void AddEntry(std::string name, Entry::Units units, std::string value);

  const ContainerDump* container_dump() { return container_dump_; }
  const std::map<std::string, Entry>& entries() { return entries_; }

 private:
  ContainerDump* container_dump_;
  std::map<std::string, Entry> entries_;
  std::map<std::string, DumpNode*> children_;

  DumpEdge* owns_edge_;
  std::vector<DumpEdge*> owned_by_edges_;
};

class DumpEdge {
 public:
  DumpEdge(DumpNode* source, DumpNode* target, int priority);

  const DumpNode* source() { return source_; }
  const DumpNode* target() { return target_; }
  int priority() { return priority_; }
  bool is_weak() { return weak_; }
  void set_weak(bool weak) { weak_ = weak; }

 private:
  const DumpNode* source_;
  const DumpNode* target_;
  const int priority_;
  bool weak_;
};

}  // namespace memory_instrumentation
#endif