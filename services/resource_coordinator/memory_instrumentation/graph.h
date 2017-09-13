// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_RESOURCE_COORDINATOR_MEMORY_INSTRUMENTATION_GRAPH_H_
#define SERVICES_RESOURCE_COORDINATOR_MEMORY_INSTRUMENTATION_GRAPH_H_

#include <map>
#include <memory>
#include <vector>

#include "base/process/process_handle.h"

namespace memory_instrumentation {

class DumpNode;
struct ProcessDump;

class GlobalDump {
 public:
  using ProcessDumpMap =
      std::map<base::ProcessId, std::unique_ptr<ProcessDump>>;

  GlobalDump();
  ~GlobalDump();

  ProcessDump* CreateProcessDump(base::ProcessId process_id);
  DumpNode* CreateNode(DumpNode* parent);

  const ProcessDumpMap& dumps_for_testing() { return dumps; }

 private:
  std::vector<std::unique_ptr<DumpNode>> all_nodes;
  ProcessDumpMap dumps;
  // DumpNode* global_dumps;
};

struct ProcessDump {
  ProcessDump(DumpNode* root) : root(root){};
  DumpNode* root;
  // ...
};

struct DumpEdge {
  int priority;
  bool weak;
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

    Type type;
    Units units;

    std::string value_string;
    std::uint64_t value_uint64;
  };

  DumpNode(GlobalDump* global, DumpNode* parent);
  ~DumpNode();

  DumpNode* GetChild(std::string subpath);
  DumpNode* GetOrCreateChild(std::string subpath);
  void AddEntry(std::string name, Entry::Units units, uint64_t value);
  void AddEntry(std::string name, Entry::Units units, std::string value);

  const std::map<std::string, Entry>& entries() { return entries_; }

 private:
  // (probably unnecessary as this is the key of the map)
  // std::string name_;

  // DumpNode* parent_;
  GlobalDump* global_;
  std::map<std::string, DumpNode*> children_;
  std::map<std::string, Entry> entries_;

  std::map<DumpNode*, DumpEdge> incoming_edges_;
};

}  // namespace memory_instrumentation
#endif