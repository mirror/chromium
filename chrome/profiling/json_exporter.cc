// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/profiling/json_exporter.h"

#include <map>

#include "base/strings/string_number_conversions.h"

namespace profiling {

namespace {

// Maps strings to integers for the JSON string table.
using StringTable = std::map<std::string, size_t>;

struct BacktraceNode {
  BacktraceNode(size_t sid, size_t p) : string_id(sid), parent(p) {}

  static constexpr size_t kNoParent = static_cast<size_t>(-1);

  size_t string_id;
  size_t parent;  // kNoParent indicates no parent.
};

// Used as a map kep to uniquify an allocation with a given size and stack.
// Since backtraces are uniquified, this does pointer comparisons on the
// backtrace to give a stable ordering, even if that ordering has no
// intrinsic meaning.
struct UniqueAlloc {
  UniqueAlloc(const Backtrace* bt, size_t sz) : backtrace(bt), size(sz) {}

  bool operator<(const UniqueAlloc& other) const {
    return std::tie(backtrace, size) < std::tie(other.backtrace, other.size);
  }

  const Backtrace* backtrace;
  size_t size;
};

using UniqueAllocCount = std::map<UniqueAlloc, int>;

// Only support one type, this is the type ID of that type.
constexpr int kTypeId = 0;

// In "maps":
//   "strings" is a string table, maps id -> unique string.
//   "types" maps type IDs to string IDs. A type is "[unknown]", "ipc", "gpu",
//           etc. (basically heap providers).
//   "nodes" is a set of stack trace nodes. You walk up a stack trace following
//           the "parent" IDs (which is just a reference to another node). The
//           string that corresponds to this stack is "name_sid".
//
// In "allocators":

// Writes a dummy process name entry given a PID.
void WriteProcessName(int pid, std::ostream& out) {
  out << "{ \"pid\":" << pid << ", \"ph\":\"M\", \"name\":\"process_name\", "
      << "\"args\":{\"name\":\"Browser process\"}}";
}

void WriteHeapsV2Header(int pid, std::ostream& out) {
  out << "{ \"pid\":" << pid << ",";
  out << "\"ph\":\"v\",";
  out << "\"name\":\"periodic_interval\",";
  out << "\"args\":{";
  out << "\"dumps\":{";
  out << "\"level_of_detail\":\"detailed\",";
  out << "\"heaps_v2\": {\n";
}

void WriteHeapsV2Footer(std::ostream& out) {
  out << "}}}}";  // heaps_v2, dumps, args, event
}

// Inserts or retrieves the ID for a string in the string table.
size_t AddOrGetString(std::string str, StringTable* string_table) {
  auto result = string_table->emplace(std::move(str), string_table->size());
  // "result.first" is an iterator into the map.
  return result.first->second;
}

// Returns the index into nodes of the one to reference for this stack.
size_t AppendBacktraceStrings(const Backtrace& backtrace,
                              std::vector<BacktraceNode>* nodes,
                              StringTable* string_table) {
  int parent = -1;
  for (const auto& addr : backtrace.addrs()) {
    size_t sid =
        AddOrGetString("pc:" + base::Uint64ToString(addr.value), string_table);
    nodes->emplace_back(sid, parent);
    parent = nodes->size() - 1;
  }
  return nodes->size() - 1;  // Last item is the end of this stack.
}

void WriteStrings(const StringTable& string_table, std::ostream& out) {
  out << "\"strings\":[";
  bool first_time = true;
  for (const auto& string_pair : string_table) {
    if (!first_time)
      out << ",\n";
    first_time = false;

    out << "{\"id\":" << string_pair.second;
    // TODO(brettw) when we have real symbols this will need escaping.
    out << ",\"string\":\"" << string_pair.first << "\"}";
  }
  out << "]";
}

void WriteMapNodes(const std::vector<BacktraceNode>& nodes, std::ostream& out) {
  out << "\"nodes\":[";

  for (size_t i = 0; i < nodes.size(); i++) {
    if (i != 0)
      out << ",\n";

    out << "{\"id\":" << i;
    out << ",\"name_sid\":" << nodes[i].string_id;
    if (nodes[i].parent != BacktraceNode::kNoParent)
      out << ",\"parent\":" << nodes[i].parent;
    out << "}";
  }
  out << "]";
}

void WriteCounts(const UniqueAllocCount& alloc_counts, std::ostream& out) {
  out << "\"counts\":[";
  bool first_time = true;
  for (const auto& cur : alloc_counts) {
    if (!first_time)
      out << ",\n";
    first_time = false;
    out << cur.second;
  }
  out << "]";
}

void WriteSizes(const UniqueAllocCount& alloc_counts, std::ostream& out) {
  out << "\"sizes\":[";
  bool first_time = true;
  for (const auto& cur : alloc_counts) {
    if (!first_time)
      out << ",\n";
    first_time = false;
    out << cur.first.size;
  }
  out << "]";
}

void WriteTypes(const UniqueAllocCount& alloc_counts, std::ostream& out) {
  out << "\"types\":[";
  for (size_t i = 0; i < alloc_counts.size(); i++) {
    if (i != 0)
      out << ",";
    out << kTypeId;
  }
  out << "]";
}

void WriteAllocatorNodes(const UniqueAllocCount& alloc_counts,
                         const std::map<const Backtrace*, size_t>& backtraces,
                         std::ostream& out) {
  out << "\"nodes\":[";
  bool first_time = true;
  for (const auto& cur : alloc_counts) {
    if (!first_time)
      out << ",\n";
    first_time = false;
    auto found = backtraces.find(cur.first.backtrace);
    out << found->second;
  }
  out << "]";
}

}  // namespace

void ExportAllocationEventSetToJSON(int pid,
                                    const BacktraceStorage* backtrace_storage,
                                    const AllocationEventSet& set,
                                    std::ostream& out) {
  out << "{ \"traceEvents\": [";
  WriteProcessName(pid, out);
  out << ",\n";
  WriteHeapsV2Header(pid, out);

  StringTable string_table;

  // We hardcode one type, "[unknown]".
  size_t type_string_id = AddOrGetString("[unknown]", &string_table);

  // Find all backtraces referenced by the set. The backtrace storage will
  // contain more stacks than we want to write out (it will refer to all
  // processes, while we're only writing one). So do those only on demand.
  //
  // The map maps backtrace keys to node IDs (computed below).
  std::map<const Backtrace*, size_t> backtraces;
  for (const auto& event : set) {
    backtraces.emplace(
        &backtrace_storage->GetBacktraceForKey(event.backtrace_key()), 0);
  }

  // Write each backtrace, converting the string for the stack entry to string
  // IDs. The backtrace_key -> node ID will be filled in at this time.
  //
  // As a future optimization, compute when a stack is a superset of another
  // one and share the common nodes.
  std::vector<BacktraceNode> nodes;
  nodes.reserve(backtraces.size() * 10);  // Guesstimate for end size.
  for (auto& bt : backtraces) {
    bt.second = AppendBacktraceStrings(*bt.first, &nodes, &string_table);
  }

  // Maps section.
  out << "\"maps\": {\n";
  WriteStrings(string_table, out);
  out << ",\n";
  WriteMapNodes(nodes, out);
  out << ",\n";
  out << "\"types\":[{\"id\":" << kTypeId << ",\"name_sid\":" << type_string_id
      << "}]";
  out << "},\n";  // End of maps section.

  // Aggregate allocations. Allocations of the same size and stack get grouped.
  UniqueAllocCount alloc_counts;
  for (const auto& alloc : set) {
    alloc_counts[UniqueAlloc(
        &backtrace_storage->GetBacktraceForKey(alloc.backtrace_key()),
        alloc.size())]++;
  }

  // Allocators section.
  out << "\"allocators\":{\"malloc\":{\n";
  WriteCounts(alloc_counts, out);
  out << ",\n";
  WriteSizes(alloc_counts, out);
  out << ",\n";
  WriteTypes(alloc_counts, out);
  out << ",\n";
  WriteAllocatorNodes(alloc_counts, backtraces, out);
  out << "}}\n";  // End of allocators section.

  WriteHeapsV2Footer(out);
  out << "]}\n";
}

}  // namespace profiling
