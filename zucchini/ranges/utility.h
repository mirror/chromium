// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ZUCCHINI_RANGES_UTILITY_H_
#define ZUCCHINI_RANGES_UTILITY_H_

#include <iterator>
#include <type_traits>
#include <utility>

#include "zucchini/ranges/meta.h"

namespace zucchini {
namespace ranges {

using std::begin;
using std::end;
#ifdef WIN32
using std::make_reverse_iterator;
using std::cbegin;
using std::cend;
using std::rbegin;
using std::rend;
#else
template <class Iterator>
constexpr std::reverse_iterator<Iterator> make_reverse_iterator(Iterator i) {
  return std::reverse_iterator<Iterator>{i};
}

template <class C>
constexpr auto cbegin(const C& c) -> decltype(std::begin(c)) {
  return std::begin(c);
}
template <class C>
constexpr auto cend(const C& c) -> decltype(std::end(c)) {
  return std::end(c);
}
template <class C>
constexpr auto rbegin(C& c) -> decltype(c.rbegin()) {  // NOLINT for C&.
  return c.rbegin();
}
template <class C>
constexpr auto rend(C& c) -> decltype(c.rend()) {  // NOLINT.
  return c.rend();
}
#endif
template <class C>
constexpr auto crbegin(const C& c) -> decltype(rbegin(c)) {
  return rbegin(c);
}
template <class C>
constexpr auto crend(const C& c) -> decltype(rend(c)) {
  return rend(c);
}

template <class Rng, class Default = size_t>
struct DefaultedSizeType {
  template <class U>
  static typename U::size_type Size();
  template <class U, class... E>
  static Default Size(E...);
  using type = decltype(Size<typename std::remove_reference<Rng>::type>());
};

template <class Rng>
struct range_traits {
  using iterator = decltype(begin(std::declval<Rng&>()));
  using sentinel = decltype(end(std::declval<Rng&>()));
  using const_iterator = decltype(cbegin(std::declval<Rng&>()));
  using const_sentinel = decltype(cend(std::declval<Rng&>()));
  using value_type = typename std::iterator_traits<iterator>::value_type;
  using size_type = typename DefaultedSizeType<Rng>::type;
  using difference_type =
      typename std::iterator_traits<iterator>::difference_type;
  using reference = typename std::iterator_traits<iterator>::reference;
  using const_reference =
      typename std::iterator_traits<const_iterator>::reference;
  using pointer = typename std::iterator_traits<iterator>::pointer;
  using const_pointer = typename std::iterator_traits<const_iterator>::pointer;
};

/******** distance() ********/

namespace detail {

template <
    class Iterator,
    class Sentinel,
    class Difference = typename std::iterator_traits<Iterator>::difference_type>
Difference distance_impl(Iterator first,
                         Sentinel last,
                         std::random_access_iterator_tag) {
  return last - first;
}

template <
    class Iterator,
    class Sentinel,
    class Difference = typename std::iterator_traits<Iterator>::difference_type>
Difference distance_impl(Iterator first,
                         Sentinel last,
                         std::input_iterator_tag) {
  Difference i = 0;
  while (first != last) {
    ++i;
    ++first;
  }
  return i;
}

}  // namespace detail

template <
    class Iterator,
    class Sentinel,
    class Difference = typename std::iterator_traits<Iterator>::difference_type>
Difference distance(Iterator first, Sentinel last) {
  return detail::distance_impl(
      first, last,
      typename std::iterator_traits<Iterator>::iterator_category());
}

/******** empty() ********/

template <class Rng>
bool empty(Rng&& rng) {
  auto first = begin(rng);
  auto last = end(rng);
  return first == last;
}

/******** size() ********/

namespace detail {

template <class Rng, class E = decltype(std::declval<Rng>().size())>
typename range_traits<Rng>::size_type size_impl(Rng&& rng) {
  return rng.size();
}

template <class Rng, class... E>
typename range_traits<Rng>::size_type size_impl(Rng&& rng, E...) {
  return distance(begin(rng), end(rng));
}

}  // namespace detail

template <class Rng>
typename range_traits<Rng>::size_type size(Rng&& rng) {
  return detail::size_impl(std::forward<Rng>(rng));
}

/******** equal() ********/

namespace detail {

// Both Rng1 and Rng2 have size(), presumably O(1). We use it to quickly detect
// size mismatches, or to simplify element-by-element comparisons loop.
template <class Rng1,
          class Rng2,
          class E1 = decltype(std::declval<Rng1>().size()),
          class E2 = decltype(std::declval<Rng2>().size())>
bool equal_impl(Rng1&& r1, Rng2&& r2) {
  size_t common_size = static_cast<size_t>(r1.size());
  if (common_size != static_cast<size_t>(r2.size()))
    return false;
  auto it1 = begin(r1);
  auto it2 = begin(r2);
  for (size_t i = 0; i < common_size; ++i)
    if (!(*(it1++) == *(it2++)))
      return false;
  return true;
}

// Rng1 and/or Rng2 do not have size(). Since range::size() may be inefficient,
// so just perform element-by-element comparisons directly.
template <class Rng1, class Rng2, class... E>
bool equal_impl(Rng1&& r1, Rng2&& r2, E...) {
  auto it1 = begin(r1);
  auto it2 = begin(r2);
  while (it1 != end(r1) && it2 != end(r2)) {
    if (!(*(it1++) == *(it2++)))
      return false;
  }
  return it1 == end(r1) && it2 == end(r2);
}

}  // namespace detail

template <class Rng1, class Rng2>
bool equal(Rng1&& a, Rng2&& b) {
  return detail::equal_impl(a, b);
}

/******** CheckRet() ********/

template <class Ret>
void CheckRet() {
  static_assert(!std::is_rvalue_reference<Ret>::value,
                "Return value should not be an rvalue reference");
}

}  // namespace ranges
}  // namespace zucchini

#endif  // ZUCCHINI_RANGES_UTILITY_H_
