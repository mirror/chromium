// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ZUCCHINI_RANGES_ALGORITHM_H_
#define ZUCCHINI_RANGES_ALGORITHM_H_

#include <algorithm>
#include <functional>
#include <utility>

#include "zucchini/ranges/functional.h"
#include "zucchini/ranges/utility.h"
#include "zucchini/ranges/view.h"

namespace zucchini {
namespace ranges {

template <class Rng>
void sort(Rng&& rng) {
  std::sort(begin(rng), end(rng));
}

template <class Rng, class Compare, class Proj = identity>
void sort(Rng&& rng, Compare comp, Proj proj = {}) {
  using std::placeholders::_1;
  using std::placeholders::_2;
  auto proj_ = as_function(std::move(proj));
  std::sort(begin(rng), end(rng),
            std::bind(comp, std::bind(proj_, _1), std::bind(proj_, _2)));
}

template <class Rng>
typename range_traits<Rng>::iterator max_element(Rng&& rng) {
  return std::max_element(begin(rng), end(rng));
}

template <class Rng, class Compare, class Proj = identity>
typename range_traits<Rng>::iterator max_element(Rng&& rng,
                                                 Compare comp,
                                                 Proj proj = {}) {
  using std::placeholders::_1;
  using std::placeholders::_2;
  auto proj_ = as_function(proj);
  return std::max_element(
      begin(rng), end(rng),
      std::bind(comp, std::bind(proj_, _1), std::bind(proj_, _2)));
}

template <class Rng>
RngFacade<view::DropView<Rng>> unique(Rng&& rng) {
  return RngFacade<view::DropView<Rng>>{begin(rng),
                                        std::unique(begin(rng), end(rng))};
}

template <class Rng, class BinaryPred, class Proj = identity>
RngFacade<view::DropView<Rng>> unique(Rng&& rng,
                                      BinaryPred pred,
                                      Proj proj = {}) {
  using std::placeholders::_1;
  auto proj_ = as_function(std::move(proj));
  return RngFacade<view::DropView<Rng>>{
      begin(rng),
      std::unique(begin(rng), end(rng), std::bind(pred, std::bind(proj_, _1)))};
}

template <class InputIt, class Sentinel, class OutputIt>
OutputIt copy(InputIt first, Sentinel last, OutputIt d_first) {
  while (first != last) {
    *d_first = *first;
    ++d_first;
    ++first;
  }
  return d_first;
}

template <class InputRng, class OutputIt>
OutputIt copy(InputRng&& input, OutputIt output) {
  return copy(begin(input), end(input), output);
}

template <class InputRng,
          class OutputIt,
          class UnaryPredicate,
          class Proj = identity>
OutputIt copy_if(InputRng&& input,
                 OutputIt output,
                 UnaryPredicate pred,
                 Proj proj = {}) {
  using std::placeholders::_1;
  auto proj_ = as_function(std::move(proj));
  return copy_if(begin(input), end(input), output,
                 std::bind(pred, std::bind(proj_, _1)));
}

template <class InputRng, class OutputIt, class UnaryOp>
OutputIt transform(InputRng&& input, OutputIt output, UnaryOp op) {
  auto op_ = as_function(std::move(op));
  return std::transform(begin(input), end(input), output, std::move(op_));
}

template <class InputRng, class T>
typename range_traits<InputRng>::iterator lower_bound(InputRng&& input,
                                                      const T& value) {
  return std::lower_bound(begin(input), end(input), value);
}

template <class InputRng, class T, class Compare, class Proj = identity>
typename range_traits<InputRng>::iterator lower_bound(InputRng&& input,
                                                      const T& value,
                                                      Compare comp,
                                                      Proj proj = {}) {
  using std::placeholders::_1;
  using std::placeholders::_2;
  auto proj_ = as_function(std::move(proj));
  return std::lower_bound(begin(input), end(input), value,
                          std::bind(comp, std::bind(proj_, _1), _2));
}

template <class InputRng, class T>
typename range_traits<InputRng>::iterator upper_bound(InputRng&& input,
                                                      const T& value) {
  return std::upper_bound(begin(input), end(input), value);
}

template <class InputRng, class T, class Compare, class Proj = identity>
typename range_traits<InputRng>::iterator upper_bound(InputRng&& input,
                                                      const T& value,
                                                      Compare comp,
                                                      Proj proj = {}) {
  using std::placeholders::_1;
  using std::placeholders::_2;
  auto proj_ = as_function(std::move(proj));
  return std::upper_bound(begin(input), end(input), value,
                          std::bind(comp, _1, std::bind(proj_, _2)));
}

template <class ForwardIt, class T, class Compare, class Proj = identity>
ForwardIt upper_bound(ForwardIt first,
                      ForwardIt last,
                      const T& value,
                      Compare comp,
                      Proj proj = {}) {
  using std::placeholders::_1;
  using std::placeholders::_2;
  auto proj_ = as_function(std::move(proj));
  return std::upper_bound(first, last, value,
                          std::bind(comp, _1, std::bind(proj_, _2)));
}

template <class InputIt, class Sentinel, class UnaryFunction>
UnaryFunction&& for_each(InputIt first, Sentinel last, UnaryFunction&& f) {
  for (; first != last; ++first) {
    f(*first);
  }
  return f;
}

template <class Rng, class UnaryFunction>
UnaryFunction&& for_each(Rng&& input, UnaryFunction&& f) {
  // <algorithm> defines for_each(), so need to explicitly specify namespace!
  return zucchini::ranges::for_each(begin(input), end(input), f);
}

template <class InputIt1, class InputIt2>
std::pair<InputIt1, InputIt2> mismatch(InputIt1 first1,
                                       InputIt1 last1,
                                       InputIt2 first2,
                                       InputIt2 last2) {
  while (first1 != last1 && first2 != last2 && *first1 == *first2) {
    ++first1, ++first2;
  }
  return std::make_pair(first1, first2);
}

}  // namespace ranges
}  // namespace zucchini

#endif  // ZUCCHINI_RANGES_ALGORITHM_H_
