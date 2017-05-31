// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "zucchini/ranges/meta.h"

#include <tuple>
#include <type_traits>

#include "testing/gtest/include/gtest/gtest.h"

namespace zucchini {
namespace ranges {

TEST(RangesMetaTest, Transform) {
  using input = std::tuple<int, float&, double&&>;
  using output = meta::transform_t<input, std::decay>;
  using expected = std::tuple<int, float, double>;
  static_assert(std::is_same<output, expected>::value, "");
}

template <class T1, class T2>
struct common_type : std::common_type<T1, T2> {};

TEST(RangesMetaTest, Reduce) {
  using input = std::tuple<int, float, double>;
  using output = meta::reduce_t<input, unsigned, common_type>;
  using expected = double;
  static_assert(std::is_same<output, expected>::value, "");
}

TEST(RangesMetaTest, Index) {
  using input = std::tuple<int, float, double>;
  static_assert(meta::index<input, double>::value == 2, "");
}

TEST(RangesMetaTest, Contains) {
  using input = std::tuple<int, float, double>;
  static_assert(meta::contains<input, double>::value, "");
  static_assert(!meta::contains<input, unsigned>::value, "");
}

TEST(RangesMetaTest, Any) {
  using input = std::tuple<int, float, double>;
  static_assert(meta::any<input, std::is_floating_point>::value, "");
  static_assert(!meta::any<input, std::is_pointer>::value, "");
}

TEST(RangesMetaTest, None) {
  using input = std::tuple<int, float, double>;
  static_assert(!meta::none<input, std::is_floating_point>::value, "");
  static_assert(meta::none<input, std::is_pointer>::value, "");
}

TEST(RangesMetaTest, All) {
  using input = std::tuple<int, float, double>;
  static_assert(meta::all<input, std::is_arithmetic>::value, "");
  static_assert(!meta::all<input, std::is_floating_point>::value, "");
}

TEST(RangesMetaTest, CountIf) {
  using input = std::tuple<int, float, double>;
  static_assert(
      meta::count_if<input, std::is_floating_point>::value == unsigned(2), "");
}

}  // namespace ranges
}  // namespace zucchini
