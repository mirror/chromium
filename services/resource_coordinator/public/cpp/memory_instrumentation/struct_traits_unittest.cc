// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <utility>

#include "base/memory/ptr_util.h"
#include "base/memory/ref_counted.h"
#include "base/trace_event/heap_profiler_serialization_state.h"
#include "base/trace_event/memory_dump_request_args.h"
#include "base/trace_event/process_memory_dump.h"
#include "services/resource_coordinator/public/cpp/memory_instrumentation/memory_instrumentation_struct_traits.h"
#include "services/resource_coordinator/public/interfaces/memory_instrumentation/memory_instrumentation.mojom.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace memory_instrumentation {

using base::trace_event::MemoryAllocatorDump;
using MemoryAllocatorDumpEdge =
    base::trace_event::ProcessMemoryDump::MemoryAllocatorDumpEdge;
using base::trace_event::MemoryAllocatorDumpGuid;
using base::trace_event::MemoryDumpRequestArgs;
using base::trace_event::MemoryDumpArgs;
using base::trace_event::MemoryDumpLevelOfDetail;
using base::trace_event::ProcessMemoryDump;
using base::trace_event::HeapProfilerSerializationState;
using base::trace_event::StackFrameDeduplicator;
using base::trace_event::TypeNameDeduplicator;
using base::trace_event::MemoryDumpType;

using testing::Eq;
using testing::Contains;
using testing::ByRef;

namespace {

using StructTraitsTest = testing::Test;

// Test StructTrait serialization and deserialization for copyable type. |input|
// will be serialized and then deserialized into |output|.
template <class MojomType, class Type>
void SerializeAndDeserialize(const Type& input, Type* output) {
  MojomType::Deserialize(MojomType::Serialize(&input), output);
}

}  // namespace

TEST_F(StructTraitsTest, MemoryDumpRequestArgs) {
  MemoryDumpRequestArgs input{10u, MemoryDumpType::SUMMARY_ONLY,
                              MemoryDumpLevelOfDetail::DETAILED};
  MemoryDumpRequestArgs output;
  SerializeAndDeserialize<mojom::RequestArgs>(input, &output);
  EXPECT_EQ(output.dump_guid, 10u);
  EXPECT_EQ(output.dump_type, MemoryDumpType::SUMMARY_ONLY);
  EXPECT_EQ(output.level_of_detail, MemoryDumpLevelOfDetail::DETAILED);
}

TEST_F(StructTraitsTest, MemoryDumpArgs) {
  MemoryDumpArgs input{MemoryDumpLevelOfDetail::BACKGROUND};
  MemoryDumpArgs output;
  SerializeAndDeserialize<mojom::Args>(input, &output);
  EXPECT_EQ(output.level_of_detail, MemoryDumpLevelOfDetail::BACKGROUND);
}

TEST_F(StructTraitsTest, MemoryAllocatorDumpGuid) {
  MemoryAllocatorDumpGuid input(42);
  MemoryAllocatorDumpGuid output;
  SerializeAndDeserialize<mojom::AllocatorDumpGuid>(input, &output);
  EXPECT_EQ(output, MemoryAllocatorDumpGuid(42));
}

TEST_F(StructTraitsTest, MemoryAllocatorDumpEdge) {
  MemoryAllocatorDumpGuid source(1);
  MemoryAllocatorDumpGuid target(2);
  int importance = 2;
  bool overridable = true;
  MemoryAllocatorDumpEdge input{source, target, importance, overridable};
  MemoryAllocatorDumpEdge output;
  SerializeAndDeserialize<mojom::AllocatorDumpEdge>(input, &output);
  EXPECT_EQ(source, output.source);
  EXPECT_EQ(target, output.target);
  EXPECT_EQ(importance, output.importance);
  EXPECT_EQ(overridable, output.overridable);
}

TEST_F(StructTraitsTest, ProcessMemoryDump) {
  auto heap_state = base::MakeRefCounted<HeapProfilerSerializationState>();
  heap_state->SetStackFrameDeduplicator(
      base::MakeUnique<StackFrameDeduplicator>());
  heap_state->SetTypeNameDeduplicator(base::MakeUnique<TypeNameDeduplicator>());

  MemoryDumpArgs args{MemoryDumpLevelOfDetail::DETAILED};
  ProcessMemoryDump input = ProcessMemoryDump(heap_state, args);

  auto weakGuid = MemoryAllocatorDumpGuid(1);
  auto* weakMad = input.CreateWeakSharedGlobalAllocatorDump(weakGuid);
  weakMad->AddScalar("one", "oneUnit", 1);

  auto* mad1 = input.CreateAllocatorDump("mad1");
  mad1->AddScalar(base::trace_event::MemoryAllocatorDump::kNameSize,
                  base::trace_event::MemoryAllocatorDump::kUnitsBytes, 1024);
  mad1->AddScalar("one", "integer", 1);
  mad1->AddString("two", "string", "2");
  auto expected_mad1_guid = mad1->guid();
  input.CreateAllocatorDump("mad2");
  input.AddOwnershipEdge(MemoryAllocatorDumpGuid(42),
                         MemoryAllocatorDumpGuid(4242));
  ProcessMemoryDump output;

  SerializeAndDeserialize<mojom::RawProcessMemoryDump>(input, &output);

  EXPECT_EQ(MemoryDumpLevelOfDetail::DETAILED,
            output.dump_args().level_of_detail);
  EXPECT_TRUE(output.GetSharedGlobalAllocatorDump(weakGuid));
  EXPECT_EQ(1u, output.allocator_dumps().count("mad1"));
  EXPECT_EQ(1u, output.allocator_dumps().count("mad2"));
  EXPECT_EQ(1u, output.allocator_dumps_edges_for_testing().size());

  const auto& actual_mad1 = output.allocator_dumps().at("mad1");
  const std::vector<MemoryAllocatorDump::Entry>& entries =
      actual_mad1->entries_for_testing();

  {
    MemoryAllocatorDump::Entry expected("one", "integer", 1);
    EXPECT_THAT(entries, Contains(Eq(ByRef(expected))));
  }
  {
    MemoryAllocatorDump::Entry expected("two", "string", "2");
    EXPECT_THAT(entries, Contains(Eq(ByRef(expected))));
  }
  EXPECT_EQ(1024U, actual_mad1->GetSizeInternal());
  EXPECT_EQ(expected_mad1_guid, actual_mad1->guid());

  // HeapProfilerSerializationState is not preserved.
  EXPECT_NE(heap_state.get(), output.heap_profiler_serialization_state().get());
}

}  // namespace memory_instrumentation
