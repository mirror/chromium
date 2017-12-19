// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/layout/ng/floats/ng_exclusion_space.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

namespace {

#define TEST_OPPORTUNITY(opp, expected_start_offset, expected_end_offset) \
  EXPECT_EQ(expected_start_offset, opp.rect.start_offset);                \
  EXPECT_EQ(expected_end_offset, opp.rect.end_offset)

#define EXCLUSION(offset, size, type)                                        \
  NGExclusion::Create(                                                       \
      NGBfcRect(offset, NGBfcOffset(offset.line_offset + size.inline_size,   \
                                    offset.block_offset + size.block_size)), \
      type)

TEST(NGExclusionSpaceTest, Empty) {
  NGExclusionSpace exclusion_space;

  Vector<NGLayoutOpportunity> opportunites =
      exclusion_space.AllLayoutOpportunities(
          /* offset */ {LayoutUnit(), LayoutUnit()},
          /* available_size */ LayoutUnit(100));

  EXPECT_EQ(1u, opportunites.size());
  TEST_OPPORTUNITY(opportunites[0], NGBfcOffset(LayoutUnit(), LayoutUnit()),
                   NGBfcOffset(LayoutUnit(100), LayoutUnit::Max()));

  // TODO WHAT HAPPENS WITH -VE INLINE_SIZE?
  opportunites = exclusion_space.AllLayoutOpportunities(
      /* offset */ {LayoutUnit(-30), LayoutUnit(-100)},
      /* available_size */ LayoutUnit(50));

  EXPECT_EQ(1u, opportunites.size());
  TEST_OPPORTUNITY(opportunites[0],
                   NGBfcOffset(LayoutUnit(-30), LayoutUnit(-100)),
                   NGBfcOffset(LayoutUnit(20), LayoutUnit::Max()));

  opportunites = exclusion_space.AllLayoutOpportunities(
      /* offset */ {LayoutUnit(30), LayoutUnit(100)},
      /* available_size */ LayoutUnit(50));

  EXPECT_EQ(1u, opportunites.size());
  TEST_OPPORTUNITY(opportunites[0],
                   NGBfcOffset(LayoutUnit(30), LayoutUnit(100)),
                   NGBfcOffset(LayoutUnit(80), LayoutUnit::Max()));
}

TEST(NGExclusionSpaceTest, SingleExclusion) {
  NGExclusionSpace exclusion_space;

  exclusion_space.Add(EXCLUSION(NGBfcOffset(LayoutUnit(10), LayoutUnit(20)),
                                NGLogicalSize(LayoutUnit(50), LayoutUnit(70)),
                                EFloat::kLeft));

  Vector<NGLayoutOpportunity> opportunites =
      exclusion_space.AllLayoutOpportunities(
          /* offset */ {LayoutUnit(), LayoutUnit()},
          /* available_size */ LayoutUnit(100));

  EXPECT_EQ(3u, opportunites.size());
  TEST_OPPORTUNITY(opportunites[0], NGBfcOffset(LayoutUnit(), LayoutUnit()),
                   NGBfcOffset(LayoutUnit(100), LayoutUnit(20)));
  TEST_OPPORTUNITY(opportunites[1], NGBfcOffset(LayoutUnit(60), LayoutUnit()),
                   NGBfcOffset(LayoutUnit(100), LayoutUnit::Max()));
  TEST_OPPORTUNITY(opportunites[2], NGBfcOffset(LayoutUnit(), LayoutUnit(90)),
                   NGBfcOffset(LayoutUnit(100), LayoutUnit::Max()));

  opportunites = exclusion_space.AllLayoutOpportunities(
      /* offset */ {LayoutUnit(-10), LayoutUnit(-100)},
      /* available_size */ LayoutUnit(100));

  EXPECT_EQ(3u, opportunites.size());
  TEST_OPPORTUNITY(opportunites[0],
                   NGBfcOffset(LayoutUnit(-10), LayoutUnit(-100)),
                   NGBfcOffset(LayoutUnit(90), LayoutUnit(20)));
  TEST_OPPORTUNITY(opportunites[1],
                   NGBfcOffset(LayoutUnit(60), LayoutUnit(-100)),
                   NGBfcOffset(LayoutUnit(90), LayoutUnit::Max()));
  TEST_OPPORTUNITY(opportunites[2],
                   NGBfcOffset(LayoutUnit(-10), LayoutUnit(90)),
                   NGBfcOffset(LayoutUnit(90), LayoutUnit::Max()));

  // This will only produce two opportunities, as the RHS opportunity will be
  // outside the search area.
  opportunites = exclusion_space.AllLayoutOpportunities(
      /* offset */ {LayoutUnit(10), LayoutUnit(10)},
      /* available_size */ LayoutUnit(50));

  EXPECT_EQ(2u, opportunites.size());
  TEST_OPPORTUNITY(opportunites[0], NGBfcOffset(LayoutUnit(10), LayoutUnit(10)),
                   NGBfcOffset(LayoutUnit(60), LayoutUnit(20)));
  TEST_OPPORTUNITY(opportunites[1], NGBfcOffset(LayoutUnit(10), LayoutUnit(90)),
                   NGBfcOffset(LayoutUnit(60), LayoutUnit::Max()));
}

TEST(NGExclusionSpaceTest, TwoExclusions) {
  NGExclusionSpace exclusion_space;

  exclusion_space.Add(EXCLUSION(NGBfcOffset(LayoutUnit(), LayoutUnit()),
                                NGLogicalSize(LayoutUnit(150), LayoutUnit(75)),
                                EFloat::kLeft));
  exclusion_space.Add(EXCLUSION(NGBfcOffset(LayoutUnit(100), LayoutUnit(75)),
                                NGLogicalSize(LayoutUnit(300), LayoutUnit(75)),
                                EFloat::kRight));

  Vector<NGLayoutOpportunity> opportunites =
      exclusion_space.AllLayoutOpportunities(
          /* offset */ {LayoutUnit(), LayoutUnit()},
          /* available_size */ LayoutUnit(400));

  EXPECT_EQ(3u, opportunites.size());
  TEST_OPPORTUNITY(opportunites[0], NGBfcOffset(LayoutUnit(150), LayoutUnit()),
                   NGBfcOffset(LayoutUnit(400), LayoutUnit(75)));
  TEST_OPPORTUNITY(opportunites[1], NGBfcOffset(LayoutUnit(), LayoutUnit(75)),
                   NGBfcOffset(LayoutUnit(100), LayoutUnit::Max()));
  TEST_OPPORTUNITY(opportunites[2], NGBfcOffset(LayoutUnit(), LayoutUnit(150)),
                   NGBfcOffset(LayoutUnit(400), LayoutUnit::Max()));
}

}  // namespace
}  // namespace blink
