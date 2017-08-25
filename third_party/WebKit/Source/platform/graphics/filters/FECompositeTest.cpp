// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/graphics/filters/FEComposite.h"

#include "platform/geometry/LayoutRect.h"
#include "platform/graphics/filters/FEOffset.h"
#include "platform/graphics/filters/Filter.h"
#include "platform/graphics/filters/SourceGraphic.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

class FECompositeTest : public ::testing::Test {
 protected:
  FEComposite* CreateComposite(CompositeOperationType type,
                               float k1 = 0,
                               float k2 = 0,
                               float k3 = 0,
                               float k4 = 0) {
    //          FEComposite
    //          /         \
    // FEOffset(50,0) FEOffset(0,-50)
    //        |             |
    //  SourceGraphic  SourceGraphic
    //
    auto* filter =
        Filter::Create(FloatRect(), FloatRect(LayoutRect::InfiniteIntRect()), 1,
                       Filter::kUserSpace);
    auto* composite = FEComposite::Create(filter, type, k1, k2, k3, k4);
    composite->SetClipsToBounds(false);

    auto* source_graphic = SourceGraphic::Create(filter);
    source_graphic->SetClipsToBounds(false);

    auto* offset1 = FEOffset::Create(filter, 50, 0);
    offset1->SetClipsToBounds(false);
    offset1->InputEffects().push_back(source_graphic);
    composite->InputEffects().push_back(offset1);

    auto* offset2 = FEOffset::Create(filter, 0, -50);
    offset2->SetClipsToBounds(false);
    offset2->InputEffects().push_back(source_graphic);
    composite->InputEffects().push_back(offset2);

    return composite;
  }
};

#define EXPECT_INTERSECTION(c)                                \
  EXPECT_TRUE(c->MapRect(FloatRect()).IsEmpty());             \
  EXPECT_TRUE(c->MapRect(FloatRect(0, 0, 50, 50)).IsEmpty()); \
  EXPECT_EQ(FloatRect(50, 0, 150, 150), c->MapRect(FloatRect(0, 0, 200, 200)));

#define EXPECT_INPUT1(c)                                                    \
  EXPECT_TRUE(c->MapRect(FloatRect()).IsEmpty());                           \
  EXPECT_EQ(FloatRect(50, 0, 50, 50), c->MapRect(FloatRect(0, 0, 50, 50))); \
  EXPECT_EQ(FloatRect(50, 0, 200, 200), c->MapRect(FloatRect(0, 0, 200, 200)));

#define EXPECT_INPUT2(c)                                                     \
  EXPECT_TRUE(c->MapRect(FloatRect()).IsEmpty());                            \
  EXPECT_EQ(FloatRect(0, -50, 50, 50), c->MapRect(FloatRect(0, 0, 50, 50))); \
  EXPECT_EQ(FloatRect(0, -50, 200, 200), c->MapRect(FloatRect(0, 0, 200, 200)));

#define EXPECT_UNION(c)                                                        \
  EXPECT_TRUE(c->MapRect(FloatRect()).IsEmpty());                              \
  EXPECT_EQ(FloatRect(0, -50, 100, 100), c->MapRect(FloatRect(0, 0, 50, 50))); \
  EXPECT_EQ(FloatRect(0, -50, 250, 250), c->MapRect(FloatRect(0, 0, 200, 200)));

#define EXPECT_EMPTY(c)                                       \
  EXPECT_TRUE(c->MapRect(FloatRect()).IsEmpty());             \
  EXPECT_TRUE(c->MapRect(FloatRect(0, 0, 50, 50)).IsEmpty()); \
  EXPECT_TRUE(c->MapRect(FloatRect(0, 0, 200, 200)).IsEmpty());

TEST_F(FECompositeTest, MapRectIn) {
  EXPECT_INTERSECTION(CreateComposite(FECOMPOSITE_OPERATOR_IN));
}

TEST_F(FECompositeTest, MapRectATop) {
  EXPECT_INPUT2(CreateComposite(FECOMPOSITE_OPERATOR_ATOP));
}

TEST_F(FECompositeTest, MapRectOtherOperators) {
  EXPECT_UNION(CreateComposite(FECOMPOSITE_OPERATOR_OVER));
  EXPECT_UNION(CreateComposite(FECOMPOSITE_OPERATOR_OUT));
  EXPECT_UNION(CreateComposite(FECOMPOSITE_OPERATOR_XOR));
  EXPECT_UNION(CreateComposite(FECOMPOSITE_OPERATOR_LIGHTER));
}

TEST_F(FECompositeTest, MapRectArithmetic) {
  EXPECT_EMPTY(CreateComposite(FECOMPOSITE_OPERATOR_ARITHMETIC, 0, 0, 0, 0));
  EXPECT_UNION(CreateComposite(FECOMPOSITE_OPERATOR_ARITHMETIC, 0, 0, 0, 1));
  EXPECT_INPUT2(CreateComposite(FECOMPOSITE_OPERATOR_ARITHMETIC, 0, 0, 1, 0));
  EXPECT_UNION(CreateComposite(FECOMPOSITE_OPERATOR_ARITHMETIC, 0, 0, 1, 1));
  EXPECT_INPUT1(CreateComposite(FECOMPOSITE_OPERATOR_ARITHMETIC, 0, 1, 0, 0));
  EXPECT_UNION(CreateComposite(FECOMPOSITE_OPERATOR_ARITHMETIC, 0, 1, 0, 1));
  EXPECT_UNION(CreateComposite(FECOMPOSITE_OPERATOR_ARITHMETIC, 0, 1, 1, 0));
  EXPECT_UNION(CreateComposite(FECOMPOSITE_OPERATOR_ARITHMETIC, 0, 1, 1, 1));
  EXPECT_INTERSECTION(
      CreateComposite(FECOMPOSITE_OPERATOR_ARITHMETIC, 1, 0, 0, 0));
  EXPECT_UNION(CreateComposite(FECOMPOSITE_OPERATOR_ARITHMETIC, 1, 0, 0, 1));
  EXPECT_INPUT2(CreateComposite(FECOMPOSITE_OPERATOR_ARITHMETIC, 1, 0, 1, 0));
  EXPECT_UNION(CreateComposite(FECOMPOSITE_OPERATOR_ARITHMETIC, 1, 0, 1, 1));
  EXPECT_INPUT1(CreateComposite(FECOMPOSITE_OPERATOR_ARITHMETIC, 1, 1, 0, 0));
  EXPECT_UNION(CreateComposite(FECOMPOSITE_OPERATOR_ARITHMETIC, 1, 1, 0, 1));
  EXPECT_UNION(CreateComposite(FECOMPOSITE_OPERATOR_ARITHMETIC, 1, 1, 1, 0));
  EXPECT_UNION(CreateComposite(FECOMPOSITE_OPERATOR_ARITHMETIC, 1, 1, 1, 1));
}

TEST_F(FECompositeTest, MapRectArithmeticK4Clipped) {
  // Arithmetic operator with positive K4 will always affect the whole primitive
  // subregion.
  auto* c = CreateComposite(FECOMPOSITE_OPERATOR_ARITHMETIC, 1, 1, 1, 1);
  c->SetClipsToBounds(true);
  FloatRect bounds(222, 333, 444, 555);
  c->SetFilterPrimitiveSubregion(bounds);
  EXPECT_EQ(bounds, c->MapRect(FloatRect()));
  EXPECT_EQ(bounds, c->MapRect(FloatRect(100, 200, 300, 400)));
}

}  // namespace blink
