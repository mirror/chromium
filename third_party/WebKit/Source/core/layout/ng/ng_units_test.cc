// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/layout/ng/ng_units.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

namespace {

TEST(NGUnitsTest, ConvertLogicalOffsetToPhysicalOffset) {
  NGLogicalOffset logical_offset(LayoutUnit(20), LayoutUnit(30));
  NGPhysicalSize outer_size(LayoutUnit(300), LayoutUnit(400));
  NGPhysicalSize inner_size(LayoutUnit(5), LayoutUnit(65));
  NGPhysicalOffset offset;

  offset = logical_offset.ConvertToPhysical(kHorizontalTopBottom, LTR,
                                            outer_size, inner_size);
  EXPECT_EQ(LayoutUnit(20), offset.left);
  EXPECT_EQ(LayoutUnit(30), offset.top);

  offset = logical_offset.ConvertToPhysical(kHorizontalTopBottom, RTL,
                                            outer_size, inner_size);
  EXPECT_EQ(LayoutUnit(275), offset.left);
  EXPECT_EQ(LayoutUnit(30), offset.top);

  offset = logical_offset.ConvertToPhysical(kVerticalRightLeft, LTR, outer_size,
                                            inner_size);
  EXPECT_EQ(LayoutUnit(265), offset.left);
  EXPECT_EQ(LayoutUnit(20), offset.top);

  offset = logical_offset.ConvertToPhysical(kVerticalRightLeft, RTL, outer_size,
                                            inner_size);
  EXPECT_EQ(LayoutUnit(265), offset.left);
  EXPECT_EQ(LayoutUnit(315), offset.top);

  offset = logical_offset.ConvertToPhysical(kSidewaysRightLeft, LTR, outer_size,
                                            inner_size);
  EXPECT_EQ(LayoutUnit(265), offset.left);
  EXPECT_EQ(LayoutUnit(20), offset.top);

  offset = logical_offset.ConvertToPhysical(kSidewaysRightLeft, RTL, outer_size,
                                            inner_size);
  EXPECT_EQ(LayoutUnit(265), offset.left);
  EXPECT_EQ(LayoutUnit(315), offset.top);

  offset = logical_offset.ConvertToPhysical(kVerticalLeftRight, LTR, outer_size,
                                            inner_size);
  EXPECT_EQ(LayoutUnit(30), offset.left);
  EXPECT_EQ(LayoutUnit(20), offset.top);

  offset = logical_offset.ConvertToPhysical(kVerticalLeftRight, RTL, outer_size,
                                            inner_size);
  EXPECT_EQ(LayoutUnit(30), offset.left);
  EXPECT_EQ(LayoutUnit(315), offset.top);

  offset = logical_offset.ConvertToPhysical(kSidewaysLeftRight, LTR, outer_size,
                                            inner_size);
  EXPECT_EQ(LayoutUnit(30), offset.left);
  EXPECT_EQ(LayoutUnit(315), offset.top);

  offset = logical_offset.ConvertToPhysical(kSidewaysLeftRight, RTL, outer_size,
                                            inner_size);
  EXPECT_EQ(LayoutUnit(30), offset.left);
  EXPECT_EQ(LayoutUnit(20), offset.top);
}

// Ideally, this would be tested by NGBoxStrut::ConvertToPhysical, but
// this has not been implemented yet.
TEST(NGUnitsTest, ConvertPhysicalStrutToLogical) {
  LayoutUnit left{5}, right{10}, top{15}, bottom{20};
  NGPhysicalBoxStrut physical{left, right, top, bottom};

  NGBoxStrut logical = physical.ConvertToLogical(kHorizontalTopBottom, LTR);
  EXPECT_EQ(left, logical.inline_start);
  EXPECT_EQ(top, logical.block_start);

  logical = physical.ConvertToLogical(kHorizontalTopBottom, RTL);
  EXPECT_EQ(right, logical.inline_start);
  EXPECT_EQ(top, logical.block_start);

  logical = physical.ConvertToLogical(kVerticalLeftRight, LTR);
  EXPECT_EQ(top, logical.inline_start);
  EXPECT_EQ(left, logical.block_start);

  logical = physical.ConvertToLogical(kVerticalLeftRight, RTL);
  EXPECT_EQ(bottom, logical.inline_start);
  EXPECT_EQ(left, logical.block_start);

  logical = physical.ConvertToLogical(kVerticalRightLeft, LTR);
  EXPECT_EQ(top, logical.inline_start);
  EXPECT_EQ(right, logical.block_start);

  logical = physical.ConvertToLogical(kVerticalRightLeft, RTL);
  EXPECT_EQ(bottom, logical.inline_start);
  EXPECT_EQ(right, logical.block_start);
}

TEST(NGUnitsTest, ShrinkToFit) {
  MinAndMaxContentSizes sizes;

  sizes.min_content = LayoutUnit(100);
  sizes.max_content = LayoutUnit(200);
  EXPECT_EQ(LayoutUnit(200), sizes.ShrinkToFit(LayoutUnit(300)));

  sizes.min_content = LayoutUnit(100);
  sizes.max_content = LayoutUnit(300);
  EXPECT_EQ(LayoutUnit(200), sizes.ShrinkToFit(LayoutUnit(200)));

  sizes.min_content = LayoutUnit(200);
  sizes.max_content = LayoutUnit(300);
  EXPECT_EQ(LayoutUnit(200), sizes.ShrinkToFit(LayoutUnit(100)));
}

}  // namespace

}  // namespace blink
