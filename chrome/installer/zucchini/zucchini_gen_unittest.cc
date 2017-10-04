// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/zucchini/zucchini_gen.h"

#include <stdint.h>

#include <utility>
#include <vector>

#include "chrome/installer/zucchini/equivalence_map.h"
#include "chrome/installer/zucchini/image_index.h"
#include "chrome/installer/zucchini/image_utils.h"
#include "chrome/installer/zucchini/label_manager.h"
#include "chrome/installer/zucchini/test_disassembler.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace zucchini {

namespace {

constexpr auto BAD = kUnusedIndex;
using OffsetVector = std::vector<offset_t>;

// Helper function wrapping GenerateReferencesDelta().
std::vector<int32_t> GenerateReferencesDeltaTest(
    const std::vector<Reference>& old_references,
    const std::vector<Reference>& new_references,
    std::vector<offset_t>&& new_labels,
    const EquivalenceMap& equivalence_map) {
  ReferenceDeltaSink reference_delta_sink;

  TargetPool old_targets;
  old_targets.InsertTargets(old_references);
  ReferenceSet old_refs({1, TypeTag(0), PoolTag(0)});
  old_refs.InsertReferences(old_references, old_targets);

  TargetPool new_targets;
  new_targets.InsertTargets(new_references);
  ReferenceSet new_refs({1, TypeTag(0), PoolTag(0)});
  new_refs.InsertReferences(new_references, new_targets);

  UnorderedLabelManager label_manager;
  label_manager.Init(std::move(new_labels));

  GenerateReferencesDelta(old_refs, new_refs, new_targets, label_manager,
                          equivalence_map, &reference_delta_sink);

  // Serialize |reference_delta_sink| to patch format, and read it back as
  // std::vector<int32_t>.
  std::vector<uint8_t> buffer(reference_delta_sink.SerializedSize());
  BufferSink sink(buffer.data(), buffer.size());
  reference_delta_sink.SerializeInto(&sink);

  BufferSource source(buffer.data(), buffer.size());
  ReferenceDeltaSource reference_delta_source;
  EXPECT_TRUE(reference_delta_source.Initialize(&source));
  std::vector<int32_t> delta_vec;
  for (auto delta = reference_delta_source.GetNext(); delta.has_value();
       delta = reference_delta_source.GetNext()) {
    delta_vec.push_back(*delta);
  }
  EXPECT_TRUE(reference_delta_source.Done());
  return delta_vec;
}

// Helper function wrapping FindExtraTargets().
std::vector<offset_t> FindExtraTargetsTest(
    std::vector<Reference>&& new_references,
    std::vector<offset_t>&& new_labels,
    EquivalenceMap&& equivalence_map) {
  TargetPool new_targets;
  new_targets.InsertTargets(new_references);
  ReferenceSet reference_set({1, TypeTag(0), PoolTag(0)});
  reference_set.InsertReferences(new_references, new_targets);

  UnorderedLabelManager new_label_manager;
  new_label_manager.Init(std::move(new_labels));
  return FindExtraTargets(reference_set, new_targets, new_label_manager,
                          equivalence_map);
}

}  // namespace

TEST(ZucchiniGenTest, MakeNewTargetsFromEquivalenceMap) {
  // Note that |old_offsets| provided are sorted, and |equivalences| provided
  // are sorted by |src_offset|.

  EXPECT_EQ(OffsetVector(), MakeNewTargetsFromEquivalenceMap({}, {}));
  EXPECT_EQ(OffsetVector({BAD, BAD}),
            MakeNewTargetsFromEquivalenceMap({0, 1}, {}));

  EXPECT_EQ(OffsetVector({0, 1, BAD}),
            MakeNewTargetsFromEquivalenceMap({0, 1, 2}, {{0, 0, 2}}));
  EXPECT_EQ(OffsetVector({1, 2, BAD}),
            MakeNewTargetsFromEquivalenceMap({0, 1, 2}, {{0, 1, 2}}));
  EXPECT_EQ(OffsetVector({1, BAD, 4, 5, 6, BAD}),
            MakeNewTargetsFromEquivalenceMap({0, 1, 2, 3, 4, 5},
                                             {{0, 1, 1}, {2, 4, 3}}));
  EXPECT_EQ(OffsetVector({3, BAD, 0, 1, 2, BAD}),
            MakeNewTargetsFromEquivalenceMap({0, 1, 2, 3, 4, 5},
                                             {{0, 3, 1}, {2, 0, 3}}));

  // Overlap in src.
  EXPECT_EQ(OffsetVector({1, 2, 3, BAD, BAD}),
            MakeNewTargetsFromEquivalenceMap({0, 1, 2, 3, 4},
                                             {{0, 1, 3}, {1, 4, 2}}));
  EXPECT_EQ(OffsetVector({1, 4, 5, 6, BAD}),
            MakeNewTargetsFromEquivalenceMap({0, 1, 2, 3, 4},
                                             {{0, 1, 2}, {1, 4, 3}}));
  EXPECT_EQ(OffsetVector({1, 2, 5, BAD, BAD}),
            MakeNewTargetsFromEquivalenceMap({0, 1, 2, 3, 4},
                                             {{0, 1, 2}, {1, 4, 2}}));

  // Jump in src.
  EXPECT_EQ(OffsetVector({5, BAD, 6}),
            MakeNewTargetsFromEquivalenceMap(
                {10, 13, 15}, {{0, 1, 2}, {9, 4, 2}, {15, 6, 2}}));
}

TEST(ZucchiniGenTest, FindExtraTargets) {
  // Note that |new_offsets| provided are sorted, and |equivalences| provided
  // are sorted by |dst_offset|.

  EXPECT_EQ(OffsetVector(), FindExtraTargetsTest({}, {}, {}));
  EXPECT_EQ(OffsetVector(), FindExtraTargetsTest({}, {}, {}));
  EXPECT_EQ(OffsetVector(), FindExtraTargetsTest({{0, 0}}, {}, {}));
  EXPECT_EQ(OffsetVector(), FindExtraTargetsTest({{0, 0}}, {kUnusedIndex}, {}));
  EXPECT_EQ(OffsetVector(),
            FindExtraTargetsTest({{0, 0}, {3, 0}}, {},
                                 EquivalenceMap({{{2, 1, 2}, 0.0}})));

  EXPECT_EQ(
      OffsetVector({1}),
      FindExtraTargetsTest({{0, 1}}, {}, EquivalenceMap({{{0, 0, 2}, 0.0}})));
  EXPECT_EQ(OffsetVector({1}),
            FindExtraTargetsTest({{0, 1}}, {kUnusedIndex},
                                 EquivalenceMap({{{0, 0, 2}, 0.0}})));
  EXPECT_EQ(
      OffsetVector({}),
      FindExtraTargetsTest({{0, 1}}, {1}, EquivalenceMap({{{0, 0, 2}, 0.0}})));

  EXPECT_EQ(OffsetVector({1, 2}),
            FindExtraTargetsTest({{0, 0}, {1, 1}, {2, 2}, {3, 3}}, {},
                                 EquivalenceMap({{{0, 1, 2}, 0.0}})));
  EXPECT_EQ(
      OffsetVector({1, 2}),
      FindExtraTargetsTest({{0, 0}, {1, 1}, {2, 2}, {3, 3}}, {kUnusedIndex, 11},
                           EquivalenceMap({{{0, 1, 2}, 0.0}})));

  EXPECT_EQ(OffsetVector({1, 2, 4, 5}),
            FindExtraTargetsTest(
                {{0, 0}, {1, 1}, {2, 2}, {3, 3}, {4, 4}, {5, 5}, {6, 6}},
                {kUnusedIndex},
                EquivalenceMap({{{0, 1, 2}, 0.0}, {{0, 4, 2}, 0.0}})));
  EXPECT_EQ(OffsetVector({2, 4}),
            FindExtraTargetsTest(
                {{0, 0}, {1, 1}, {2, 2}, {3, 3}, {4, 4}, {5, 5}, {6, 6}},
                {5, kUnusedIndex, 1},
                EquivalenceMap({{{0, 1, 2}, 0.0}, {{0, 4, 2}, 0.0}})));
  EXPECT_EQ(OffsetVector({4, 5}),
            FindExtraTargetsTest(
                {{3, 3}, {4, 4}, {5, 5}, {6, 6}}, {},
                EquivalenceMap({{{0, 1, 2}, 0.0}, {{0, 4, 2}, 0.0}})));
}

TEST(ZucchiniGenTest, GenerateReferencesDelta) {
  EXPECT_EQ(std::vector<int32_t>(),
            GenerateReferencesDeltaTest({}, {}, {}, EquivalenceMap()));

  EXPECT_EQ(
      std::vector<int32_t>(),
      GenerateReferencesDeltaTest({{0, 0}}, {{0, 0}}, {}, EquivalenceMap()));

  EXPECT_EQ(std::vector<int32_t>({0}),
            GenerateReferencesDeltaTest({{0, 0}}, {{0, 0}}, {0, 1},
                                        EquivalenceMap({{{0, 0, 4}, 0.0}})));
  EXPECT_EQ(std::vector<int32_t>({1}),
            GenerateReferencesDeltaTest({{0, 0}}, {{0, 0}}, {1, 0},
                                        EquivalenceMap({{{0, 0, 4}, 0.0}})));
  EXPECT_EQ(
      std::vector<int32_t>({2, -1}),
      GenerateReferencesDeltaTest({{0, 0}, {1, 1}}, {{0, 0}, {1, 1}}, {1, 2, 0},
                                  EquivalenceMap({{{0, 0, 4}, 0.0}})));

  EXPECT_EQ(std::vector<int32_t>({0, 0}),
            GenerateReferencesDeltaTest({{0, 0}, {1, 1}, {2, 2}, {3, 3}},
                                        {{0, 0}, {1, 1}, {2, 2}, {3, 3}},
                                        {0, 1, 2, 3},
                                        EquivalenceMap({{{1, 1, 2}, 0.0}})));

  EXPECT_EQ(std::vector<int32_t>({-1, 1}),
            GenerateReferencesDeltaTest(
                {{0, 0}, {2, 1}}, {{0, 0}, {2, 1}}, {0, 1},
                EquivalenceMap({{{2, 0, 2}, 0.0}, {{0, 2, 2}, 0.0}})));
  EXPECT_EQ(std::vector<int32_t>({0, 0}),
            GenerateReferencesDeltaTest(
                {{0, 0}, {2, 1}}, {{0, 0}, {2, 1}}, {1, 0},
                EquivalenceMap({{{2, 0, 2}, 0.0}, {{0, 2, 2}, 0.0}})));

  EXPECT_EQ(std::vector<int32_t>({-2, 2}),
            GenerateReferencesDeltaTest(
                {{0, 0}, {2, 1}, {4, 2}}, {{0, 0}, {2, 1}, {4, 2}}, {0, 1, 2},
                EquivalenceMap({{{4, 0, 2}, 0.0}, {{0, 4, 2}, 0.0}})));

  EXPECT_EQ(std::vector<int32_t>({-2, 2}),
            GenerateReferencesDeltaTest(
                {{1, 0}, {4, 1}, {7, 2}}, {{1, 0}, {4, 1}, {7, 2}}, {0, 1, 2},
                EquivalenceMap({{{6, 0, 3}, 0.0}, {{0, 6, 3}, 0.0}})));

  EXPECT_EQ(std::vector<int32_t>({-2, 2}),
            GenerateReferencesDeltaTest(
                {{0, 0}, {4, 2}, {6, 1}}, {{0, 0}, {4, 2}}, {0, 1, 2},
                EquivalenceMap(
                    {{{4, 0, 2}, 0.0}, {{2, 2, 2}, 0.0}, {{0, 4, 2}, 0.0}})));
}

// TODO(huangs): Add more tests.

}  // namespace zucchini
