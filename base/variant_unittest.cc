// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/variant.h"

#include <memory>
#include <utility>

#include "testing/gtest/include/gtest/gtest.h"

namespace base {

TEST(VariantTest, Construction) {
  // The default constructor makes the first alternative.
  variant<int> x;
  EXPECT_EQ(0u, x.index());

  // Construct with value. nullptr_t should match to std::unique_ptr<int>.
  variant<int, std::unique_ptr<int>> y(nullptr);
  EXPECT_EQ(1u, y.index());
  EXPECT_EQ(nullptr, get<1>(y));

  // Copy.
  variant<int> x2(x);
  EXPECT_EQ(0u, x2.index());

  // Move.
  variant<int, std::unique_ptr<int>> y2 = std::move(y);
  EXPECT_EQ(1u, y2.index());

  // Give the type explicitly.
  variant<int, char, double> z(in_place_type_t<double>(), 1);
  EXPECT_EQ(2u, z.index());
  EXPECT_EQ(1.0, get<double>(z));

  // Give the index explicitly.
  variant<int, char, double> z2(in_place_index_t<1>(), 'a');
  EXPECT_EQ(1u, z2.index());
  EXPECT_EQ('a', get<char>(z2));
}

TEST(VariantTest, Assignment) {
  variant<int> x;
  x = 1;
  EXPECT_EQ(0u, x.index());
  EXPECT_EQ(1, get<int>(x));

  variant<int> y;
  y = x;
  EXPECT_EQ(1, get<int>(y));
  x = 2;
  y = std::move(x);
  EXPECT_EQ(2, get<int>(y));

  variant<std::unique_ptr<int>, int> z;
  z = nullptr;
  EXPECT_EQ(0u, z.index());
  EXPECT_EQ(nullptr, get<std::unique_ptr<int>>(z));

  variant<std::unique_ptr<int>, int> w;
  w = std::move(z);
  EXPECT_EQ(0u, w.index());
  EXPECT_EQ(nullptr, get<std::unique_ptr<int>>(w));

  z.emplace<std::unique_ptr<int>>(new int);
  w.emplace<1>(1);
  EXPECT_EQ(0u, z.index());
  EXPECT_EQ(1u, w.index());
  swap(z, w);
  EXPECT_EQ(1u, z.index());
  EXPECT_EQ(0u, w.index());

  EXPECT_EQ(1, get<int>(z));
  EXPECT_TRUE(get<std::unique_ptr<int>>(w));
}

TEST(VariantTest, Accessor) {
  variant<int, double, char, std::unique_ptr<int>> x;
  EXPECT_EQ(4u, variant_size<decltype(x)>::value);

  x.emplace<double>();
  EXPECT_TRUE(holds_alternative<double>(x));
  EXPECT_FALSE(holds_alternative<int>(x));

  variant<int> y = 1;
  EXPECT_EQ(1, get<int>(y));
  EXPECT_EQ(1, get<int>(const_cast<const variant<int>&>(y)));
  EXPECT_EQ(1, get<0>(y));
  EXPECT_EQ(1, get<0>(const_cast<const variant<int>&>(y)));

  EXPECT_EQ(1, get<int>(std::move(y)));
  EXPECT_EQ(1, get<int>(std::move(const_cast<const variant<int>&>(y))));
  EXPECT_EQ(1, get<0>(std::move(y)));
  EXPECT_EQ(1, get<0>(std::move(const_cast<const variant<int>&>(y))));
}

TEST(VariantTest, Compare) {
  variant<int, double> x = 2;
  variant<int, double> y = 1.0;
  EXPECT_TRUE(holds_alternative<int>(x));
  EXPECT_TRUE(holds_alternative<double>(y));
  EXPECT_LT(x, y);
  EXPECT_LE(x, y);
  EXPECT_GT(y, x);
  EXPECT_GE(y, x);
  EXPECT_NE(y, x);
  y = x;
  EXPECT_EQ(y, x);
}

}  // namespace base
