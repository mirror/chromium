// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/resource_coordinator/public/cpp/memory_instrumentation/memory_instrumentation_struct_traits.h"

#include "base/trace_event/memory_dump_request_args.h"
#include "mojo/common/common_custom_types_struct_traits.h"
#include "services/resource_coordinator/public/interfaces/memory_instrumentation/memory_instrumentation.mojom.h"

namespace mojo {

// static
memory_instrumentation::mojom::DumpType
EnumTraits<memory_instrumentation::mojom::DumpType,
           base::trace_event::MemoryDumpType>::
    ToMojom(base::trace_event::MemoryDumpType type) {
  switch (type) {
    case base::trace_event::MemoryDumpType::PERIODIC_INTERVAL:
      return memory_instrumentation::mojom::DumpType::PERIODIC_INTERVAL;
    case base::trace_event::MemoryDumpType::EXPLICITLY_TRIGGERED:
      return memory_instrumentation::mojom::DumpType::EXPLICITLY_TRIGGERED;
    case base::trace_event::MemoryDumpType::PEAK_MEMORY_USAGE:
      return memory_instrumentation::mojom::DumpType::PEAK_MEMORY_USAGE;
    case base::trace_event::MemoryDumpType::SUMMARY_ONLY:
      return memory_instrumentation::mojom::DumpType::SUMMARY_ONLY;
    case base::trace_event::MemoryDumpType::VM_REGIONS_ONLY:
      return memory_instrumentation::mojom::DumpType::VM_REGIONS_ONLY;
    default:
      CHECK(false) << "Invalid type: " << static_cast<uint8_t>(type);
      // This should not be reached. Just return a random value.
      return memory_instrumentation::mojom::DumpType::PEAK_MEMORY_USAGE;
  }
}

// static
bool EnumTraits<memory_instrumentation::mojom::DumpType,
                base::trace_event::MemoryDumpType>::
    FromMojom(memory_instrumentation::mojom::DumpType input,
              base::trace_event::MemoryDumpType* out) {
  switch (input) {
    case memory_instrumentation::mojom::DumpType::PERIODIC_INTERVAL:
      *out = base::trace_event::MemoryDumpType::PERIODIC_INTERVAL;
      break;
    case memory_instrumentation::mojom::DumpType::EXPLICITLY_TRIGGERED:
      *out = base::trace_event::MemoryDumpType::EXPLICITLY_TRIGGERED;
      break;
    case memory_instrumentation::mojom::DumpType::PEAK_MEMORY_USAGE:
      *out = base::trace_event::MemoryDumpType::PEAK_MEMORY_USAGE;
      break;
    case memory_instrumentation::mojom::DumpType::SUMMARY_ONLY:
      *out = base::trace_event::MemoryDumpType::SUMMARY_ONLY;
      break;
    case memory_instrumentation::mojom::DumpType::VM_REGIONS_ONLY:
      *out = base::trace_event::MemoryDumpType::VM_REGIONS_ONLY;
      break;
    default:
      NOTREACHED() << "Invalid type: " << static_cast<uint8_t>(input);
      return false;
  }
  return true;
}

// static
memory_instrumentation::mojom::LevelOfDetail
EnumTraits<memory_instrumentation::mojom::LevelOfDetail,
           base::trace_event::MemoryDumpLevelOfDetail>::
    ToMojom(base::trace_event::MemoryDumpLevelOfDetail level_of_detail) {
  switch (level_of_detail) {
    case base::trace_event::MemoryDumpLevelOfDetail::BACKGROUND:
      return memory_instrumentation::mojom::LevelOfDetail::BACKGROUND;
    case base::trace_event::MemoryDumpLevelOfDetail::LIGHT:
      return memory_instrumentation::mojom::LevelOfDetail::LIGHT;
    case base::trace_event::MemoryDumpLevelOfDetail::DETAILED:
      return memory_instrumentation::mojom::LevelOfDetail::DETAILED;
    default:
      CHECK(false) << "Invalid type: " << static_cast<uint8_t>(level_of_detail);
      // This should not be reached. Just return a random value.
      return memory_instrumentation::mojom::LevelOfDetail::BACKGROUND;
  }
}

// static
bool EnumTraits<memory_instrumentation::mojom::LevelOfDetail,
                base::trace_event::MemoryDumpLevelOfDetail>::
    FromMojom(memory_instrumentation::mojom::LevelOfDetail input,
              base::trace_event::MemoryDumpLevelOfDetail* out) {
  switch (input) {
    case memory_instrumentation::mojom::LevelOfDetail::BACKGROUND:
      *out = base::trace_event::MemoryDumpLevelOfDetail::BACKGROUND;
      break;
    case memory_instrumentation::mojom::LevelOfDetail::LIGHT:
      *out = base::trace_event::MemoryDumpLevelOfDetail::LIGHT;
      break;
    case memory_instrumentation::mojom::LevelOfDetail::DETAILED:
      *out = base::trace_event::MemoryDumpLevelOfDetail::DETAILED;
      break;
    default:
      NOTREACHED() << "Invalid type: " << static_cast<uint8_t>(input);
      return false;
  }
  return true;
}

// static
bool StructTraits<memory_instrumentation::mojom::RequestArgsDataView,
                  base::trace_event::MemoryDumpRequestArgs>::
    Read(memory_instrumentation::mojom::RequestArgsDataView input,
         base::trace_event::MemoryDumpRequestArgs* out) {
  out->dump_guid = input.dump_guid();
  if (!input.ReadDumpType(&out->dump_type))
    return false;
  if (!input.ReadLevelOfDetail(&out->level_of_detail))
    return false;
  return true;
}

// static
bool StructTraits<memory_instrumentation::mojom::ArgsDataView,
                  base::trace_event::MemoryDumpArgs>::
    Read(memory_instrumentation::mojom::ArgsDataView input,
         base::trace_event::MemoryDumpArgs* out) {
  if (!input.ReadLevelOfDetail(&out->level_of_detail))
    return false;
  return true;
}

using base::trace_event::MemoryAllocatorDump;
using Entry = base::trace_event::MemoryAllocatorDump::Entry;
using base::trace_event::MemoryDumpArgs;
using base::trace_event::ProcessMemoryDump;
using MemoryAllocatorDumpEdge =
    base::trace_event::ProcessMemoryDump::MemoryAllocatorDumpEdge;
using base::trace_event::MemoryAllocatorDumpGuid;
using memory_instrumentation::mojom::AllocatorDumpPtr;
using memory_instrumentation::mojom::AllocatorDump;
using memory_instrumentation::mojom::EntryDataView;
using memory_instrumentation::mojom::EntryValue;
using memory_instrumentation::mojom::EntryValueDataView;
using memory_instrumentation::mojom::RawProcessMemoryDumpDataView;
using memory_instrumentation::mojom::AllocatorDumpGuidDataView;
using memory_instrumentation::mojom::AllocatorDumpEdgeDataView;

// static
bool StructTraits<AllocatorDumpGuidDataView, MemoryAllocatorDumpGuid>::Read(
    AllocatorDumpGuidDataView input,
    MemoryAllocatorDumpGuid* out) {
  out->SetGuidForSerialization(input.guid());
  return true;
}

// static
bool StructTraits<EntryDataView, Entry>::Read(EntryDataView input, Entry* out) {
  if (!input.ReadName(&out->name))
    return false;
  if (!input.ReadUnits(&out->units))
    return false;
  if (!input.ReadValue(out))
    return false;
  return true;
}

// static
bool UnionTraits<EntryValueDataView, Entry>::Read(
    EntryValueDataView data,
    MemoryAllocatorDump::Entry* out) {
  switch (data.tag()) {
    case EntryValue::Tag::VALUE_STRING: {
      std::string value_string;
      if (!data.ReadValueString(&value_string))
        return false;
      out->value_string = std::move(value_string);
      out->entry_type = Entry::EntryType::kString;
      break;
    }
    case EntryValue::Tag::VALUE_UINT64: {
      out->value_uint64 = data.value_uint64();
      out->entry_type = Entry::EntryType::kUint64;
      break;
    }
    default:
      return false;
  }
  return true;
}

bool StructTraits<AllocatorDumpEdgeDataView, MemoryAllocatorDumpEdge>::Read(
    AllocatorDumpEdgeDataView input,
    MemoryAllocatorDumpEdge* out) {
  if (!input.ReadSource(&out->source))
    return false;
  if (!input.ReadTarget(&out->target))
    return false;
  out->importance = input.importance();
  out->overridable = input.overridable();
  return true;
}

// static
std::unordered_map<std::string, AllocatorDumpPtr>
StructTraits<RawProcessMemoryDumpDataView, ProcessMemoryDump>::allocator_dumps(
    const ProcessMemoryDump& pmd) {
  std::unordered_map<std::string, AllocatorDumpPtr> results;
  for (const auto& kv : pmd.allocator_dumps()) {
    const std::unique_ptr<MemoryAllocatorDump>& mad = kv.second;
    results[mad->absolute_name()] = AllocatorDump::New();
    AllocatorDump* dump = results[mad->absolute_name()].get();
    dump->weak = mad->flags() && MemoryAllocatorDump::Flags::WEAK;
    dump->entries = mad->TakeEntriesForSerialization();
  }
  return results;
}

// static
std::vector<MemoryAllocatorDumpEdge> StructTraits<
    RawProcessMemoryDumpDataView,
    ProcessMemoryDump>::allocator_dump_edges(const ProcessMemoryDump& pmd) {
  return pmd.TakeEdgesForSerialization();
}

// static
bool StructTraits<RawProcessMemoryDumpDataView, ProcessMemoryDump>::Read(
    RawProcessMemoryDumpDataView input,
    ProcessMemoryDump* out) {
  MemoryDumpArgs dump_args;
  if (!input.ReadDumpArgs(&dump_args))
    return false;
  out->SetDumpArgsForSerialization(dump_args);
  std::vector<MemoryAllocatorDumpEdge> edges;
  if (!input.ReadAllocatorDumpEdges(&edges))
    return false;
  out->SetEdgesForSerialization(edges);

  std::unordered_map<std::string, AllocatorDumpPtr> allocator_dumps;
  if (!input.ReadAllocatorDumps(&allocator_dumps))
    return false;
  for (const auto& kv : allocator_dumps) {
    const std::string& name = kv.first;
    const AllocatorDumpPtr& dump = kv.second;
    MemoryAllocatorDump* mad = out->CreateAllocatorDump(name);
    //    dump->guid = mad->guid();
    if (dump->weak)
      mad->set_flags(MemoryAllocatorDump::Flags::WEAK);
    mad->SetEntriesForSerialization(std::move(dump->entries));
  }
  return true;
}

}  // namespace mojo
