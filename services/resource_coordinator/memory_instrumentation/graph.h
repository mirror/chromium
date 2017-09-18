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

  ContainerDump* global_container() { return global_container_; }
  const GuidNodeMap& nodes_by_guid() { return nodes_by_guid_; }
  const ContainerDumpMap& container_dumps() { return dumps_; }

 private:
  DumpNode* CreateNode(ContainerDump* container_dump);

  std::vector<std::unique_ptr<DumpNode>> all_nodes_;
  GuidNodeMap nodes_by_guid_;
  ContainerDump* global_container_;
  ContainerDumpMap dumps_;
};

struct DumpEdge {
  int priority;
  bool weak;
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

  void AddEntry(std::string name, Entry::Units units, uint64_t value);
  void AddEntry(std::string name, Entry::Units units, std::string value);

  const ContainerDump* container_dump() { return container_dump_; }
  const std::map<std::string, Entry>& entries() { return entries_; }

 private:
  ContainerDump* container_dump_;
  std::map<std::string, Entry> entries_;
  std::map<std::string, DumpNode*> children_;
  std::map<DumpNode*, DumpEdge> incoming_edges_;
};

}  // namespace memory_instrumentation
#endif