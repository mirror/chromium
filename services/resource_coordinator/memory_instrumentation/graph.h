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

// Contains processed dump graphs for each process and in the global space.
// This class is also the arena which owns the nodes of the graph.
class GlobalDump {
  friend class ContainerDump;

 public:
  using ContainerDumpMap =
      std::map<base::ProcessId, std::unique_ptr<ContainerDump>>;
  using GuidNodeMap = std::map<MemoryAllocatorDumpGuid, DumpNode*>;

  GlobalDump();
  ~GlobalDump();

  // Creates a container for all the dump graphs for the process given
  // by the given |process_id|.
  ContainerDump* CreateContainerForProcess(base::ProcessId process_id);

  // Inserts a mapping between the given |guid| and |node|.
  void InsertNodeToGuidMap(const MemoryAllocatorDumpGuid& guid, DumpNode* node);

  ContainerDump* global_container() { return global_container_; }
  const GuidNodeMap& nodes_by_guid() { return nodes_by_guid_; }
  const ContainerDumpMap& container_dumps() { return dumps_; }

 private:
  // Internal function for creating a node in the arena which is asspcoated
  // with the given |container_dump|.
  DumpNode* CreateNode(ContainerDump* container_dump);

  std::vector<std::unique_ptr<DumpNode>> all_nodes_;
  GuidNodeMap nodes_by_guid_;
  ContainerDump* global_container_;
  ContainerDumpMap dumps_;
};

// Container for a graph of dumps either associated with a process or with
// the global space.
class ContainerDump {
 public:
  ContainerDump(GlobalDump* global_dump);
  ~ContainerDump();

  // Creates a node in the dump graph which is associated with the
  // given |path|.
  DumpNode* CreateNodeInGraph(const std::string& path);

  // Returns the node in the graph at the given |path| or nullptr
  // if no such node exists.
  DumpNode* FindNodeInGraph(const std::string& path);

 private:
  GlobalDump* global_dump_;
  DumpNode* root_;
};

// A single node in the graph of allocator dumps associated with a
// certain path and containing the entries for this path.
class DumpNode {
 public:
  // Auxilary data (a scalar number or a string) about this node each
  // associated with a key.
  struct Entry {
    // Enum of the type of the entry; eiter a scalar or a string.
    enum Type {
      kUInt64,
      kString,
    };

    // The units of the entry if the entry is a scalar. The scalar
    // refers to either a number of objects or a size in bytes.
    enum Units {
      kObjects,
      kBytes,
    };

    // Creates the entry with the appropriate type.
    Entry(Units units, uint64_t value);
    Entry(Units units, std::string value);

    // The type of the entry; either a scalar or string.
    const Type type;

    // The units of the entry.
    const Units units;

    // The value of the entry if this entry has a string type.
    const std::string value_string;

    // The value of the entry if this entry has a scalar type.
    const std::uint64_t value_uint64;
  };

  DumpNode(ContainerDump* container_dump);
  ~DumpNode();

  // Gets the direct child of a node for the given |subpath|.
  DumpNode* GetChild(std::string subpath);

  // Inserts the given |node| as a child of the current node
  // with the given |subpath| as the key.
  void InsertChild(std::string subpath, DumpNode* node);

  // Adds an entry for this dump node with the given |name|, |units| and
  // type.
  void AddEntry(std::string name, Entry::Units units, uint64_t value);
  void AddEntry(std::string name, Entry::Units units, std::string value);

  const ContainerDump* container_dump() { return container_dump_; }
  const std::map<std::string, Entry>& entries() { return entries_; }

 private:
  ContainerDump* container_dump_;
  std::map<std::string, Entry> entries_;
  std::map<std::string, DumpNode*> children_;
};

}  // namespace memory_instrumentation
#endif