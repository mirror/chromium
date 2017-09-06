// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_RESOURCE_COORDINATOR_PUBLIC_CPP_MEMORY_INSTRUMENTATION_MEMORY_INSTRUMENTATION_STRUCT_TRAITS_H_
#define SERVICES_RESOURCE_COORDINATOR_PUBLIC_CPP_MEMORY_INSTRUMENTATION_MEMORY_INSTRUMENTATION_STRUCT_TRAITS_H_

#include "base/process/process_handle.h"
#include "base/trace_event/memory_dump_request_args.h"
#include "base/trace_event/process_memory_dump.h"
#include "mojo/common/common_custom_types_struct_traits.h"
#include "services/resource_coordinator/public/cpp/resource_coordinator_export.h"
#include "services/resource_coordinator/public/interfaces/memory_instrumentation/memory_instrumentation.mojom.h"

namespace mojo {

template <>
struct SERVICES_RESOURCE_COORDINATOR_PUBLIC_CPP_EXPORT
    EnumTraits<memory_instrumentation::mojom::DumpType,
               base::trace_event::MemoryDumpType> {
  static memory_instrumentation::mojom::DumpType ToMojom(
      base::trace_event::MemoryDumpType type);
  static bool FromMojom(memory_instrumentation::mojom::DumpType input,
                        base::trace_event::MemoryDumpType* out);
};

template <>
struct SERVICES_RESOURCE_COORDINATOR_PUBLIC_CPP_EXPORT
    EnumTraits<memory_instrumentation::mojom::LevelOfDetail,
               base::trace_event::MemoryDumpLevelOfDetail> {
  static memory_instrumentation::mojom::LevelOfDetail ToMojom(
      base::trace_event::MemoryDumpLevelOfDetail level_of_detail);
  static bool FromMojom(memory_instrumentation::mojom::LevelOfDetail input,
                        base::trace_event::MemoryDumpLevelOfDetail* out);
};

template <>
struct SERVICES_RESOURCE_COORDINATOR_PUBLIC_CPP_EXPORT
    StructTraits<memory_instrumentation::mojom::RequestArgsDataView,
                 base::trace_event::MemoryDumpRequestArgs> {
  static uint64_t dump_guid(
      const base::trace_event::MemoryDumpRequestArgs& args) {
    return args.dump_guid;
  }
  static base::trace_event::MemoryDumpType dump_type(
      const base::trace_event::MemoryDumpRequestArgs& args) {
    return args.dump_type;
  }
  static base::trace_event::MemoryDumpLevelOfDetail level_of_detail(
      const base::trace_event::MemoryDumpRequestArgs& args) {
    return args.level_of_detail;
  }
  static bool Read(memory_instrumentation::mojom::RequestArgsDataView input,
                   base::trace_event::MemoryDumpRequestArgs* out);
};

template <>
struct SERVICES_RESOURCE_COORDINATOR_PUBLIC_CPP_EXPORT
    StructTraits<memory_instrumentation::mojom::ArgsDataView,
                 base::trace_event::MemoryDumpArgs> {
  static base::trace_event::MemoryDumpLevelOfDetail level_of_detail(
      const base::trace_event::MemoryDumpArgs& args) {
    return args.level_of_detail;
  }
  static bool Read(memory_instrumentation::mojom::ArgsDataView input,
                   base::trace_event::MemoryDumpArgs* out);
};

template <>
struct SERVICES_RESOURCE_COORDINATOR_PUBLIC_CPP_EXPORT
    StructTraits<memory_instrumentation::mojom::AllocatorDumpGuidDataView,
                 base::trace_event::MemoryAllocatorDumpGuid> {
  static uint64_t guid(const base::trace_event::MemoryAllocatorDumpGuid& pmd) {
    return pmd.ToUint64();
  }

  static bool Read(
      memory_instrumentation::mojom::AllocatorDumpGuidDataView input,
      base::trace_event::MemoryAllocatorDumpGuid* out);
};

template <>
struct SERVICES_RESOURCE_COORDINATOR_PUBLIC_CPP_EXPORT
    StructTraits<memory_instrumentation::mojom::EntryDataView,
                 base::trace_event::MemoryAllocatorDump::Entry> {
  static std::string name(
      const base::trace_event::MemoryAllocatorDump::Entry& args) {
    return args.name;
  }
  static std::string units(
      const base::trace_event::MemoryAllocatorDump::Entry& args) {
    return args.units;
  }
  static const base::trace_event::MemoryAllocatorDump::Entry& value(
      const base::trace_event::MemoryAllocatorDump::Entry& value) {
    return value;
  }
  //  static memory_instrumentation::mojom::EntryValuePtr value(
  //      const base::trace_event::MemoryAllocatorDump::Entry& args) {
  //    auto ptr = memory_instrumentation::mojom::EntryValue::New();
  //    switch (args.entry_type) {
  //      case
  //      base::trace_event::MemoryAllocatorDump::Entry::EntryType::kUint64:
  //        ptr->set_value_uint64(args.value_uint64);
  //        break;
  //      case
  //      base::trace_event::MemoryAllocatorDump::Entry::EntryType::kString:
  //        ptr->set_value_string(std::move(args.value_string));
  //        break;
  //    }
  //    return ptr;
  //  }
  static bool Read(memory_instrumentation::mojom::EntryDataView input,
                   base::trace_event::MemoryAllocatorDump::Entry* out);
};

template <>
struct UnionTraits<memory_instrumentation::mojom::EntryValueDataView,
                   base::trace_event::MemoryAllocatorDump::Entry> {
  static memory_instrumentation::mojom::EntryValue::Tag GetTag(
      const base::trace_event::MemoryAllocatorDump::Entry& args) {
    switch (args.entry_type) {
      case base::trace_event::MemoryAllocatorDump::Entry::EntryType::kUint64:
        return memory_instrumentation::mojom::EntryValue::Tag::VALUE_UINT64;
      case base::trace_event::MemoryAllocatorDump::Entry::EntryType::kString:
        return memory_instrumentation::mojom::EntryValue::Tag::VALUE_STRING;
    }
    NOTREACHED();
    return memory_instrumentation::mojom::EntryValue::Tag::VALUE_UINT64;
  }

  static uint64_t value_uint64(
      const base::trace_event::MemoryAllocatorDump::Entry& args) {
    // DCHECK
    return args.value_uint64;
  }

  static std::string value_string(
      const base::trace_event::MemoryAllocatorDump::Entry& args) {
    return args.value_string;
  }

  static bool Read(memory_instrumentation::mojom::EntryValueDataView data,
                   base::trace_event::MemoryAllocatorDump::Entry* out);
};

template <>
struct SERVICES_RESOURCE_COORDINATOR_PUBLIC_CPP_EXPORT StructTraits<
    memory_instrumentation::mojom::AllocatorDumpEdgeDataView,
    base::trace_event::ProcessMemoryDump::MemoryAllocatorDumpEdge> {
  static base::trace_event::MemoryAllocatorDumpGuid source(
      const base::trace_event::ProcessMemoryDump::MemoryAllocatorDumpEdge&
          edge) {
    return edge.source;
  }
  static base::trace_event::MemoryAllocatorDumpGuid target(
      const base::trace_event::ProcessMemoryDump::MemoryAllocatorDumpEdge&
          edge) {
    return edge.target;
  }

  static int64_t importance(
      const base::trace_event::ProcessMemoryDump::MemoryAllocatorDumpEdge&
          edge) {
    return edge.importance;
  }

  static bool overridable(
      const base::trace_event::ProcessMemoryDump::MemoryAllocatorDumpEdge&
          edge) {
    return edge.overridable;
  }

  static bool Read(
      memory_instrumentation::mojom::AllocatorDumpEdgeDataView input,
      base::trace_event::ProcessMemoryDump::MemoryAllocatorDumpEdge* out);
};

template <>
struct SERVICES_RESOURCE_COORDINATOR_PUBLIC_CPP_EXPORT
    StructTraits<memory_instrumentation::mojom::RawProcessMemoryDumpDataView,
                 base::trace_event::ProcessMemoryDump> {
  static base::trace_event::MemoryDumpArgs dump_args(
      const base::trace_event::ProcessMemoryDump& pmd) {
    return pmd.dump_args();
  }

  static std::vector<
      base::trace_event::ProcessMemoryDump::MemoryAllocatorDumpEdge>
  allocator_dump_edges(const base::trace_event::ProcessMemoryDump& pmd);

  static std::unordered_map<std::string,
                            memory_instrumentation::mojom::AllocatorDumpPtr>
  allocator_dumps(const base::trace_event::ProcessMemoryDump& pmd);

  static bool Read(
      memory_instrumentation::mojom::RawProcessMemoryDumpDataView input,
      base::trace_event::ProcessMemoryDump* out);
};

}  // namespace mojo

#endif  // SERVICES_RESOURCE_COORDINATOR_PUBLIC_CPP_MEMORY_INSTRUMENTATION_MEMORY_INSTRUMENTATION_STRUCT_TRAITS_H_
