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
#include "base/trace_event/memory_allocator_dump_guid.h"

namespace memory_instrumentation {

class DumpNode;

// Contains processed dump graphs for each process and in the global space.
// This class is also the arena which owns the nodes of the graph.
class GlobalDumpGraph {
 public:
  // Graph of dumps either associated with a process or with
  // the shared space.
  class DumpGraph {
   public:
    DumpGraph(GlobalDumpGraph* global_graph);
    ~DumpGraph();

    // Creates a node in the dump graph which is associated with the
    // given |guid| and |path| and returns it.
    DumpNode* CreateNodeInGraph(
        const base::trace_event::MemoryAllocatorDumpGuid& guid,
        const base::StringPiece& path);

    // Returns the node in the graph at the given |path| or nullptr
    // if no such node exists in the provided |graph|.
    DumpNode* FindNodeInGraph(const base::StringPiece& path);

   private:
    GlobalDumpGraph* global_graph_;
    DumpNode* root_;
  };

  using ProcessDumpGraphMap =
      std::map<base::ProcessId, std::unique_ptr<DumpGraph>>;
  using GuidNodeMap =
      std::map<base::trace_event::MemoryAllocatorDumpGuid, DumpNode*>;

  GlobalDumpGraph();
  ~GlobalDumpGraph();

  // Creates a container for all the dump graphs for the process given
  // by the given |process_id|.
  DumpGraph* CreateGraphForProcess(base::ProcessId process_id);

  DumpGraph* shared_graph() { return shared_graph_.get(); }
  const GuidNodeMap& nodes_by_guid() { return nodes_by_guid_; }
  const ProcessDumpGraphMap& process_dump_graphs() {
    return process_dump_graphs_;
  }

 private:
  // Creates a node in the arena which is associated with the given
  // |dump_graph|.
  DumpNode* CreateNode(DumpGraph* dump_graph);

  std::vector<std::unique_ptr<DumpNode>> all_nodes_;
  GuidNodeMap nodes_by_guid_;
  std::unique_ptr<DumpGraph> shared_graph_;
  ProcessDumpGraphMap process_dump_graphs_;
};

// A single node in the graph of allocator dumps associated with a
// certain path and containing the entries for this path.
class DumpNode {
 public:
  // Auxilary data (a scalar number or a string) about this node each
  // associated with a key.
  struct Entry {
    enum Type {
      kUInt64,
      kString,
    };

    // The units of the entry if the entry is a scalar. The scalar
    // refers to either a number of objects or a size in bytes.
    enum ScalarUnits {
      kObjects,
      kBytes,
    };

    // Creates the entry with the appropriate type.
    Entry(ScalarUnits units, uint64_t value);
    Entry(ScalarUnits units, std::string value);

    const Type type;
    const ScalarUnits units;

    // The value of the entry if this entry has a string type.
    const std::string value_string;

    // The value of the entry if this entry has a integer type.
    const std::uint64_t value_uint64;
  };

  DumpNode(GlobalDumpGraph::DumpGraph* dump_graph);
  ~DumpNode();

  // Gets the direct child of a node for the given |subpath|.
  DumpNode* GetChild(const base::StringPiece& subpath);

  // Inserts the given |node| as a child of the current node
  // with the given |subpath| as the key.
  void InsertChild(const base::StringPiece& subpath, DumpNode* node);

  // Adds an entry for this dump node with the given |name|, |units| and
  // type.
  void AddEntry(std::string name, Entry::ScalarUnits units, uint64_t value);
  void AddEntry(std::string name, Entry::ScalarUnits units, std::string value);

  const GlobalDumpGraph::DumpGraph* dump_graph() { return dump_graph_; }
  const std::map<std::string, Entry>& entries() { return entries_; }

 private:
  GlobalDumpGraph::DumpGraph* dump_graph_;
  std::map<std::string, Entry> entries_;
  std::map<std::string, DumpNode*> children_;
};

}  // namespace memory_instrumentation
#endif