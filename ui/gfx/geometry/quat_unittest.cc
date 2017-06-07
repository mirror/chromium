// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <cmath>

#include "base/macros.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/geometry/quat_f.h"
#include "ui/gfx/geometry/vector3d_f.h"

namespace gfx {

TEST(QuatTest, DefaultConstruction) {
  QuatF q;
  EXPECT_FLOAT_EQ(0.0, q.x());
  EXPECT_FLOAT_EQ(0.0, q.y());
  EXPECT_FLOAT_EQ(0.0, q.z());
  EXPECT_FLOAT_EQ(1.0, q.w());
}

TEST(QuatTest, AxisAngleCommon) {
  float radians = 0.5;
  QuatF q(Vector3dF(1.0f, 0.0f, 0.0f), radians);
  EXPECT_FLOAT_EQ(std::sin(radians / 2), q.x());
  EXPECT_FLOAT_EQ(0.0, q.y());
  EXPECT_FLOAT_EQ(0.0, q.z());
  EXPECT_FLOAT_EQ(std::cos(radians / 2), q.w());
}

TEST(QuatTest, AxisAngleWithZeroLengthAxis) {
  QuatF q(Vector3dF(0.0f, 0.0f, 0.0f), 0.5);
  // If the axis of zero length, we should assume the default values.
  EXPECT_FLOAT_EQ(0.0, q.x());
  EXPECT_FLOAT_EQ(0.0, q.y());
  EXPECT_FLOAT_EQ(0.0, q.z());
  EXPECT_FLOAT_EQ(1.0, q.w());
}

TEST(QuatTest, Addition) {
  float values[] = {0.f, 1.0f, 100.0f};
  for (size_t i = 0; i < arraysize(values); ++i) {
    float t = values[i];
    QuatF a(t, 2 * t, 3 * t, 4 * t);
    QuatF b(5 * t, 4 * t, 3 * t, 2 * t);
    QuatF sum = a + b;
    EXPECT_FLOAT_EQ(6 * t, sum.x());
    EXPECT_FLOAT_EQ(6 * t, sum.y());
    EXPECT_FLOAT_EQ(6 * t, sum.z());
    EXPECT_FLOAT_EQ(6 * t, sum.w());
  }
}

TEST(QuatTest, Multiplication) {
  struct {
    QuatF a;
    QuatF b;
    QuatF expected;
  } cases[] = {
      {{1, 0, 0, 0}, {1, 0, 0, 0}, {0, 0, 0, -1}},
      {{0, 1, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, -1}},
      {{0, 0, 1, 0}, {0, 0, 1, 0}, {0, 0, 0, -1}},
      {{0, 0, 0, 1}, {0, 0, 0, 1}, {0, 0, 0, 1}},
  };

  for (size_t i = 0; i < arraysize(cases); ++i) {
    QuatF product = cases[i].a * cases[i].b;
    EXPECT_FLOAT_EQ(cases[i].expected.x(), product.x());
    EXPECT_FLOAT_EQ(cases[i].expected.y(), product.y());
    EXPECT_FLOAT_EQ(cases[i].expected.z(), product.z());
    EXPECT_FLOAT_EQ(cases[i].expected.w(), product.w());

    QuatF reverse = (cases[i].b * cases[i].a).inverse();
    EXPECT_FLOAT_EQ(reverse.x(), product.x());
    EXPECT_FLOAT_EQ(reverse.y(), product.y());
    EXPECT_FLOAT_EQ(reverse.z(), product.z());
    EXPECT_FLOAT_EQ(reverse.w(), product.w());
  }
}

TEST(QuatTest, Scaling) {
  float values[] = {0.f, 1.0f, 100.0f};
  for (size_t i = 0; i < arraysize(values); ++i) {
    QuatF q(1, 2, 3, 4);
    QuatF qs = q * values[i];
    QuatF sq = values[i] * q;
    EXPECT_FLOAT_EQ(1 * values[i], qs.x());
    EXPECT_FLOAT_EQ(2 * values[i], qs.y());
    EXPECT_FLOAT_EQ(3 * values[i], qs.z());
    EXPECT_FLOAT_EQ(4 * values[i], qs.w());
    EXPECT_FLOAT_EQ(1 * values[i], sq.x());
    EXPECT_FLOAT_EQ(2 * values[i], sq.y());
    EXPECT_FLOAT_EQ(3 * values[i], sq.z());
    EXPECT_FLOAT_EQ(4 * values[i], sq.w());
  }
}

TEST(QuatTest, Lerp) {
  for (size_t i = 0; i < 100; ++i) {
    QuatF a(0, 0, 0, 0);
    QuatF b(1, 2, 3, 4);
    float t = static_cast<float>(i) / 100.0f;
    QuatF interpolated = a.Lerp(b, t);
    EXPECT_FLOAT_EQ(1 * t, interpolated.x());
    EXPECT_FLOAT_EQ(2 * t, interpolated.y());
    EXPECT_FLOAT_EQ(3 * t, interpolated.z());
    EXPECT_FLOAT_EQ(4 * t, interpolated.w());
  }
}

}  // namespace gfx
