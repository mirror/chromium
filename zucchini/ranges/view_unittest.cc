// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "zucchini/ranges/view.h"

#include <functional>
#include <iterator>
#include <type_traits>
#include <vector>

#include "testing/gtest/include/gtest/gtest.h"
#include "zucchini/ranges/functional.h"

namespace zucchini {
namespace ranges {

template <class Iterator1, class Sentinel1, class Iterator2, class Sentinel2>
void TestIterator(Iterator1 first_expected,
                  Sentinel1 last_expected,
                  Iterator2 first_input,
                  Sentinel2 last_input,
                  std::input_iterator_tag) {
  while (first_expected != last_expected && first_input != last_input) {
    EXPECT_EQ(*first_expected, *first_input);
    ++first_expected;
    ++first_input;
  }
  EXPECT_EQ(last_input, first_input);
  EXPECT_EQ(last_expected, first_expected);
}

template <class Iterator1, class Sentinel1, class Iterator2, class Sentinel2>
void TestIterator(Iterator1 first_expected,
                  Sentinel1 last_expected,
                  Iterator2 first_input,
                  Sentinel2 last_input,
                  std::forward_iterator_tag) {
  TestIterator(first_expected, last_expected, first_input, last_input,
               std::input_iterator_tag());

  while (first_expected != last_expected && first_input != last_input) {
    EXPECT_EQ(*(first_expected++), *(first_input++));
  }
  EXPECT_EQ(last_input, first_input);
  EXPECT_EQ(last_expected, first_expected);
}

template <class Iterator1, class Sentinel1, class Iterator2, class Sentinel2>
void TestIterator(Iterator1 first_expected,
                  Sentinel1 last_expected,
                  Iterator2 first_input,
                  Sentinel2 last_input,
                  std::bidirectional_iterator_tag) {
  TestIterator(first_expected, last_expected, first_input, last_input,
               std::forward_iterator_tag());

  while (first_expected != last_expected && first_input != last_input) {
    EXPECT_EQ(*(--last_expected), *(--last_input));
  }
  EXPECT_EQ(last_input, first_input);
  EXPECT_EQ(last_expected, first_expected);
}

template <class Iterator1, class Sentinel1, class Iterator2, class Sentinel2>
void TestIterator(Iterator1 first_expected,
                  Sentinel1 last_expected,
                  Iterator2 first_input,
                  Sentinel2 last_input,
                  std::random_access_iterator_tag) {
  TestIterator(first_expected, last_expected, first_input, last_input,
               std::bidirectional_iterator_tag());

  size_t expected_size = last_expected - first_expected;
  size_t input_size = last_input - first_input;
  EXPECT_EQ(expected_size, input_size);

  for (size_t i = 0; i < expected_size; ++i) {
    EXPECT_EQ(*(first_expected + i), *(first_input + i));
    EXPECT_EQ(first_expected[i], first_input[i]);

    EXPECT_EQ(0 < i, first_input < first_input + i);
    EXPECT_EQ(0 > i, first_input > first_input + i);
    EXPECT_EQ(0 <= i, first_input <= first_input + i);
    EXPECT_EQ(0 >= i, first_input >= first_input + i);

    EXPECT_EQ(expected_size < i, last_input < first_input + i);
    EXPECT_EQ(expected_size > i, last_input > first_input + i);
    EXPECT_EQ(expected_size <= i, last_input <= first_input + i);
    EXPECT_EQ(expected_size >= i, last_input >= first_input + i);

    Iterator2 input = first_input;
    input += i;
    EXPECT_EQ(*input, first_expected[i]);
    input -= i;
    EXPECT_EQ(first_input, input);
    input += i;

    EXPECT_EQ(0 < i, first_input < input);
    EXPECT_EQ(0 > i, first_input > input);
    EXPECT_EQ(0 <= i, first_input <= input);
    EXPECT_EQ(0 >= i, first_input >= input);

    EXPECT_EQ(expected_size < i, last_input < input);
    EXPECT_EQ(expected_size > i, last_input > input);
    EXPECT_EQ(expected_size <= i, last_input <= input);
    EXPECT_EQ(expected_size >= i, last_input >= input);
  }
}

template <class Rng1, class Rng2>
void TestRange(Rng1&& expected, Rng2&& input) {
  using iterator = typename range_traits<Rng2>::iterator;
  using category = typename std::iterator_traits<iterator>::iterator_category;
  TestIterator(begin(expected), end(expected), begin(input), end(input),
               category());
}

TEST(RangesViewTest, TransformIdentity) {
  std::vector<int> input = {1, 2, 3};
  auto output = view::Transform(input, identity());
  EXPECT_TRUE(ranges::equal(input, output));

  begin(output)[0] = 0;

  TestRange(input, output);
}

TEST(RangesViewTest, Transform) {
  using std::placeholders::_1;
  std::vector<int> input = {1, 2, 3};
  auto output = view::Transform(input, std::bind(multiplies(), _1, 2));
  std::vector<int> expected = {2, 4, 6};
  TestRange(expected, output);

  input.clear();
  expected.clear();
  auto output2 = view::Transform(input, std::bind(multiplies(), _1, 2));
  TestRange(expected, output2);
}

TEST(RangesViewTest, RemoveIf) {
  using std::placeholders::_1;
  std::vector<int> input = {1, 2, 3, 1, 4, 1};
  auto output = view::RemoveIf(input, std::bind(less(), _1, 3));

  std::vector<int> expected = {3, 4};
  TestRange(expected, output);

  for (auto& i : output) {
    i = 0;
  }

  expected = {1, 2, 0, 1, 0, 1};
  TestRange(expected, input);

  input.clear();
  expected.clear();
  auto output2 = view::RemoveIf(input, std::bind(less(), _1, 3));
  TestRange(expected, output2);
}

TEST(RangesViewTest, Drop) {
  std::vector<int> input = {1, 2, 3, 1, 4, 1};
  auto output = view::Drop(input, size_t(2));
  std::vector<int> expected = {3, 1, 4, 1};
  TestRange(expected, output);
}

TEST(RangesViewTest, Iota) {
  TestRange(std::vector<int>{1, 2, 3}, view::Iota(1, 4));
}

struct MockGenerator {
  int i = 0;

  bool operator()(int* value) {
    if (i >= 5)
      return false;
    *value = i;
    ++i;
    return true;
  }
};

TEST(RangesViewTest, Generator) {
  std::vector<int> expected = {0, 1, 2, 3, 4};
  auto output = view::MakeIterable<int>(MockGenerator{});

  TestRange(expected, output);
}

}  // namespace ranges
}  // namespace zucchini
