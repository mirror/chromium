// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/profiling/json_exporter.h"

#include <map>

#include "base/strings/string_number_conversions.h"
#include "base/trace_event/trace_event_argument.h"
#include "services/resource_coordinator/public/cpp/memory_instrumentation/tracing_observer.h"

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

// Trace events support different "types" of allocations which various
// subsystems annotate stuff with for other types of tracing. For our purposes
// we only have one type, and use this ID.
constexpr int kTypeId = 0;

// Writes a dummy process name entry given a PID. When we have more information
// on a process it can be filled in here. But for now the tracing tools expect
// this entry since everything is associated with a PID.
void WriteProcessName(int pid, std::ostream& out) {
  out << "{ \"pid\":" << pid << ", \"ph\":\"M\", \"name\":\"process_name\", "
      << "\"args\":{\"name\":\"Browser process\"}}";
}

// Writes the dictionary keys to preceed a "dumps" trace argument.
void WriteDumpsHeader(int pid, std::ostream& out) {
  out << "{ \"pid\":" << pid << ",";
  out << "\"ph\":\"v\",";
  out << "\"name\":\"periodic_interval\",";
  out << "\"args\":{";
  out << "\"dumps\":{\n";
}

void WriteDumpsFooter(std::ostream& out) {
  out << "}}}";  // dumps, args, event
}

// Writes the dictionary keys to preceed a "heaps_v2" trace argument inside a
// "dumps". This is "v2" heap dump format.
void WriteHeapsV2Header(std::ostream& out) {
  out << "\"level_of_detail\":\"detailed\",\n";
  out << "\"heaps_v2\": {\n";
}

// Closes the dictionaries from the WriteHeapsV2Header function above.
void WriteHeapsV2Footer(std::ostream& out) {
  out << "}";  // heaps_v2
}

void WriteMemoryMaps(
    const std::vector<memory_instrumentation::mojom::VmRegionPtr>& maps,
    std::ostream& out) {
  base::trace_event::TracedValue traced_value;
  memory_instrumentation::TracingObserver::MemoryMapsAsValueInto(maps,
                                                                 &traced_value);
  out << "\"process_mmaps\":" << traced_value.ToString();
}

// Inserts or retrieves the ID for a string in the string table.
size_t AddOrGetString(std::string str, StringTable* string_table) {
  auto result = string_table->emplace(std::move(str), string_table->size());
  // "result.first" is an iterator into the map.
  return result.first->second;
}

// Returns the index into nodes of the node to reference for this stack. That
// node will reference its parent node, etc. to allow the full stack to
// be represented.
size_t AppendBacktraceStrings(const Backtrace& backtrace,
                              std::vector<BacktraceNode>* nodes,
                              StringTable* string_table) {
  int parent = -1;
  for (const Address& addr : backtrace.addrs()) {
    size_t sid =
        AddOrGetString("pc:" + base::Uint64ToString(addr.value), string_table);
    nodes->emplace_back(sid, parent);
    parent = nodes->size() - 1;
  }
  return nodes->size() - 1;  // Last item is the end of this stack.
}

// Writes the string table which looks like:
//   "strings":[
//     {"id":123,string:"This is the string"},
//     ...
//   ]
void WriteStrings(const StringTable& string_table, std::ostream& out) {
  out << "\"strings\":[";
  bool first_time = true;
  for (const auto& string_pair : string_table) {
    if (!first_time)
      out << ",\n";
    else
      first_time = false;

    out << "{\"id\":" << string_pair.second;
    // TODO(brettw) when we have real symbols this will need escaping.
    out << ",\"string\":\"" << string_pair.first << "\"}";
  }
  out << "]";
}

// Writes the nodes array in the maps section. These are all the backtrace
// entries and are indexed by the allocator nodes arra.
//   "nodes":[
//     {"id":1, "name_sid":123, "parent":17},
//     ...
//   ]
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

// Writes the number of matching allocations array which looks like:
//   "counts":[1, 1, 2 ]
void WriteCounts(const UniqueAllocCount& alloc_counts, std::ostream& out) {
  out << "\"counts\":[";
  bool first_time = true;
  for (const auto& cur : alloc_counts) {
    if (!first_time)
      out << ",\n";
    else
      first_time = false;
    out << cur.second;
  }
  out << "]";
}

// Writes the sizes of each allocation which looks like:
//   "sizes":[32, 64, 12]
void WriteSizes(const UniqueAllocCount& alloc_counts, std::ostream& out) {
  out << "\"sizes\":[";
  bool first_time = true;
  for (const auto& cur : alloc_counts) {
    if (!first_time)
      out << ",\n";
    else
      first_time = false;
    out << cur.first.size;
  }
  out << "]";
}

// Writes the types array of integers which looks like:
//   "types":[ 0, 0, 0, ]
void WriteTypes(const UniqueAllocCount& alloc_counts, std::ostream& out) {
  out << "\"types\":[";
  for (size_t i = 0; i < alloc_counts.size(); i++) {
    if (i != 0)
      out << ",";
    out << kTypeId;
  }
  out << "]";
}

// Writes the nodes array which indexes for each allocation into the maps nodes
// array written above. It looks like:
//   "nodes":[1, 5, 10]
void WriteAllocatorNodes(const UniqueAllocCount& alloc_counts,
                         const std::map<const Backtrace*, size_t>& backtraces,
                         std::ostream& out) {
  out << "\"nodes\":[";
  bool first_time = true;
  for (const auto& cur : alloc_counts) {
    if (!first_time)
      out << ",\n";
    else
      first_time = false;
    auto found = backtraces.find(cur.first.backtrace);
    out << found->second;
  }
  out << "]";
}

void WriteMetadata(std::ostream& out) {
 out << R"END("metadata": {"clock-domain":"LINUX_CLOCK_MONOTONIC","command_line":"/home/1337hacker/out/Debug/chrome --flag-switches-begin --memlog --enable-display-list-2d-canvas --flag-switches-end","cpu-brand":"Intel(R) Xeon(R) CPU E5-1650 v4 @ 3.60GHz","cpu-family":6,"cpu-model":79,"cpu-stepping":1,"field-trials":["ea8deb27-3f4a17df","ca7e5485-3f4a17df","241fff6c-4eda1c57","3095aa95-3f4a17df","6c43306f-ca7d8d80","7c1bc906-f55a7974","d43bf3e5-ad6ab04f","ba3f87da-45bda656","cf558fa6-48a16532","5ca89f9-ca7d8d80","f3499283-7711d854","9e201a2b-65bced95","9bd94ed7-b1c9f6b0","9773d3bd-ca7d8d80","b22b3d54-4e046809","2e109477-f3b42e62","99144bc3-4ad8d759","9e5c75f1-51efb9f9","f79cb77b-3f4a17df","b7786474-d93a0620","9591f600-d93a0620","27219e67-b2047178","23a898eb-e0e2610f","97b8b9d6-3f4a17df","48bd06b2-3f4a17df","3d7e3f6a-2eb01455","64224f74-5087fa4a","56302f8c-ca7d8d80","de03e059-e65e20f2","2697ea25-ca7d8d80","b2f0086-93053e47","494d8760-3d47f4f4","3ac60855-3ec2a267","f296190c-1facebc5","4442aae2-a90023b1","ed1d377-e1cc0f14","75f0f0a0-6bdfffe7","e2b18481-4c073154","e7e71889-4ad60575","61b920c1-ca7d8d80","828a5926-ca7d8d80"],"gpu-devid":5052,"gpu-driver":"367.57","gpu-gl-renderer":"Quadro K1200/PCIe/SSE2","gpu-gl-vendor":"NVIDIA Corporation","gpu-psver":"4.50","gpu-venid":4318,"gpu-vsver":"4.50","highres-ticks":true,"network-type":"Ethernet","num-cpus":12,"os-arch":"x86_64","os-name":"Linux","os-version":"4.4.0-83-generic","physical-memory":64322,"product-version":"Chrome/59.0.3071.115","revision":"3cf8514bb1239453fd15ff1f7efee389ac9df8ba-refs/branch-heads/3071@{#820}","trace-capture-datetime":"2017-8-9 22:56:24","trace-config":"{\"enable_argument_filter\":false,\"enable_systrace\":true,\"excluded_categories\":[\"AccountFetcherService\",\"audio\",\"benchmark\",\"blink\",\"blink_gc\",\"blink_style\",\"blink.animations\",\"blink.console\",\"blink.net\",\"blink.user_timing\",\"Blob\",\"browser\",\"CacheStorage\",\"cc\",\"cdp.perf\",\"content\",\"devtools.timeline\",\"devtools.timeline.async\",\"event\",\"FileSystem\",\"gpu\",\"gpu.capture\",\"identity\",\"IndexedDB\",\"input\",\"io\",\"ipc\",\"latencyInfo\",\"leveldb\",\"loader\",\"loading\",\"media\",\"mojom\",\"navigation\",\"net\",\"netlog\",\"omnibox\",\"p2p\",\"pepper\",\"ppapi\",\"ppapi proxy\",\"rail\",\"renderer\",\"renderer_host\",\"renderer.scheduler\",\"sandbox_ipc\",\"service_manager\",\"ServiceWorker\",\"shutdown\",\"SiteEngagement\",\"skia\",\"startup\",\"sync\",\"sync_lock_contention\",\"task_scheduler\",\"test_gpu\",\"toplevel\",\"ui\",\"v8\",\"v8.execute\",\"ValueStoreFrontend::Backend\",\"views\",\"WebCore\",\"webrtc\",\"worker.scheduler\"],\"included_categories\":[\"disabled-by-default-memory-infra\"],\"memory_dump_config\":{\"allowed_dump_modes\":[\"background\",\"light\",\"detailed\"],\"triggers\":[{\"min_time_between_dumps_ms\":2000,\"mode\":\"detailed\",\"type\":\"periodic_interval\"},{\"min_time_between_dumps_ms\":250,\"mode\":\"light\",\"type\":\"periodic_interval\"}]},\"record_mode\":\"record-until-full\"}","user-agent":"Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/59.0.3071.115 Safari/537.36","v8-version":"5.9.211.38"})END";
}

}  // namespace

void ExportAllocationEventSetToJSON(
    int pid,
    const AllocationEventSet& event_set,
    const std::vector<memory_instrumentation::mojom::VmRegionPtr>& maps,
    std::ostream& out) {
  out << "{ \"traceEvents\": [";
  WriteProcessName(pid, out);
  out << ",\n";
  WriteDumpsHeader(pid, out);

  WriteMemoryMaps(maps, out);
  out << ",\n";

  WriteHeapsV2Header(out);

  StringTable string_table;

  // We hardcode one type, "[unknown]".
  size_t type_string_id = AddOrGetString("[unknown]", &string_table);

  // Find all backtraces referenced by the set. The backtrace storage will
  // contain more stacks than we want to write out (it will refer to all
  // processes, while we're only writing one). So do those only on demand.
  //
  // The map maps backtrace keys to node IDs (computed below).
  std::map<const Backtrace*, size_t> backtraces;
  for (const auto& event : event_set)
    backtraces.emplace(event.backtrace(), 0);

  // Write each backtrace, converting the string for the stack entry to string
  // IDs. The backtrace -> node ID will be filled in at this time.
  //
  // As a future optimization, compute when a stack is a superset of another
  // one and share the common nodes.
  std::vector<BacktraceNode> nodes;
  nodes.reserve(backtraces.size() * 10);  // Guesstimate for end size.
  for (auto& bt : backtraces)
    bt.second = AppendBacktraceStrings(*bt.first, &nodes, &string_table);

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
  for (const auto& alloc : event_set) {
    UniqueAlloc unique_alloc(alloc.backtrace(), alloc.size());
    alloc_counts[unique_alloc]++;
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
  WriteDumpsFooter(out);
  out << "],";
  WriteMetadata(out);
  out << "}\n";
}

}  // namespace profiling
