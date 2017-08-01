// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_ARRAY_SLICE_H_
#define BASE_ARRAY_SLICE_H_

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <limits>
#include <type_traits>

namespace base {

template <class ElementType>
class ArraySlice {
 public:
  // --------------------------------------------------------------------------
  // Types.
  //
  using element_type = const ElementType;
  using value_type = typename std::remove_const<ElementType>::type;
  using index_type = ptrdiff_t;
  using difference_type = ptrdiff_t;
  using pointer = element_type*;
  using reference = element_type&;
  using iterator = pointer;
  using const_iterator = pointer;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  constexpr static index_type npos = std::numeric_limits<index_type>::max();

  // --------------------------------------------------------------------------
  // Constructors.
  //
  constexpr ArraySlice() noexcept;
  constexpr explicit ArraySlice(std::nullptr_t) noexcept;
  constexpr ArraySlice(pointer ptr, index_type count) noexcept;
  template <size_t N>
  constexpr ArraySlice(element_type (&arr)[N]) noexcept;
  template <class Container>
  constexpr ArraySlice(const Container& cont) noexcept;
  constexpr ArraySlice(std::initializer_list<element_type> ilist) noexcept;

  // --------------------------------------------------------------------------
  // Subslices.
  //
  constexpr ArraySlice<ElementType> first(index_type count) const noexcept;
  constexpr ArraySlice<ElementType> last(index_type count) const noexcept;
  constexpr ArraySlice<ElementType> subslice(index_type offset,
                                             index_type count = npos) const
      noexcept;

  // --------------------------------------------------------------------------
  // Observers.
  //
  constexpr index_type length() const noexcept;
  constexpr index_type size() const noexcept;
  constexpr bool empty() const noexcept;

  // --------------------------------------------------------------------------
  // Element Access.
  //
  constexpr reference operator[](index_type idx) const noexcept;
  constexpr pointer data() const noexcept;

  // --------------------------------------------------------------------------
  // Iterators.
  //
  constexpr iterator begin() const noexcept;
  constexpr iterator end() const noexcept;

  constexpr const_iterator cbegin() const noexcept;
  constexpr const_iterator cend() const noexcept;

  constexpr reverse_iterator rbegin() const noexcept;
  constexpr reverse_iterator rend() const noexcept;

  constexpr const_reverse_iterator crbegin() const noexcept;
  constexpr const_reverse_iterator crend() const noexcept;

 private:
  const pointer data_;
  const index_type size_;
};

// ----------------------------------------------------------------------------
// Comparison Operators.
//
template <class ElementType>
constexpr bool operator==(const ArraySlice<ElementType>& lhs,
                          const ArraySlice<ElementType>& rhs) noexcept;

template <class ElementType>
constexpr bool operator!=(const ArraySlice<ElementType>& lhs,
                          const ArraySlice<ElementType>& rhs) noexcept;

template <class ElementType>
constexpr bool operator<(const ArraySlice<ElementType>& lhs,
                         const ArraySlice<ElementType>& rhs) noexcept;

template <class ElementType>
constexpr bool operator>(const ArraySlice<ElementType>& lhs,
                         const ArraySlice<ElementType>& rhs) noexcept;

template <class ElementType>
constexpr bool operator<=(const ArraySlice<ElementType>& lhs,
                          const ArraySlice<ElementType>& rhs) noexcept;

template <class ElementType>
constexpr bool operator>=(const ArraySlice<ElementType>& lhs,
                          const ArraySlice<ElementType>& rhs) noexcept;

// ----------------------------------------------------------------------------
// Constructors.
//
template <class ElementType>
constexpr ArraySlice<ElementType>::ArraySlice() noexcept
    : data_(nullptr), size_(0) {}

template <class ElementType>
constexpr ArraySlice<ElementType>::ArraySlice(std::nullptr_t) noexcept
    : ArraySlice() {}

template <class ElementType>
constexpr ArraySlice<ElementType>::ArraySlice(pointer ptr,
                                              index_type count) noexcept
    : data_(ptr), size_(count) {}

template <class ElementType>
template <size_t N>
constexpr ArraySlice<ElementType>::ArraySlice(element_type (&arr)[N]) noexcept
    : ArraySlice(arr, N) {}

template <class ElementType>
template <class Container>
constexpr ArraySlice<ElementType>::ArraySlice(const Container& cont) noexcept
    : ArraySlice(cont.data(), cont.size()) {}

template <class ElementType>
constexpr ArraySlice<ElementType>::ArraySlice(
    std::initializer_list<element_type> ilist) noexcept
    : ArraySlice(ilist.begin(), ilist.size()) {}

// ----------------------------------------------------------------------------
// Subslices.
//
template <class ElementType>
constexpr ArraySlice<ElementType> ArraySlice<ElementType>::first(
    index_type count) const noexcept {
  return ArraySlice(data(), count);
}

template <class ElementType>
constexpr ArraySlice<ElementType> ArraySlice<ElementType>::last(
    index_type count) const noexcept {
  return ArraySlice(data() + (size() - count), count);
}

template <class ElementType>
constexpr ArraySlice<ElementType> ArraySlice<ElementType>::subslice(
    index_type offset,
    index_type count) const noexcept {
  return ArraySlice(data() + offset, count == npos ? size() - offset : count);
}

// ----------------------------------------------------------------------------
// Observers.
//
template <class ElementType>
constexpr typename ArraySlice<ElementType>::index_type
ArraySlice<ElementType>::length() const noexcept {
  return size();
}

template <class ElementType>
constexpr typename ArraySlice<ElementType>::index_type
ArraySlice<ElementType>::size() const noexcept {
  return size_;
}

template <class ElementType>
constexpr bool ArraySlice<ElementType>::empty() const noexcept {
  return size_ == 0;
}

// ----------------------------------------------------------------------------
// Element Access.
//
template <class ElementType>
constexpr typename ArraySlice<ElementType>::reference ArraySlice<ElementType>::
operator[](index_type idx) const noexcept {
  return data_[idx];
}

template <class ElementType>
constexpr typename ArraySlice<ElementType>::pointer
ArraySlice<ElementType>::data() const noexcept {
  return data_;
}

// ----------------------------------------------------------------------------
// Iterators.
//
template <class ElementType>
constexpr typename ArraySlice<ElementType>::iterator
ArraySlice<ElementType>::begin() const noexcept {
  return data_;
}

template <class ElementType>
constexpr typename ArraySlice<ElementType>::iterator
ArraySlice<ElementType>::end() const noexcept {
  return data_ + size_;
}

template <class ElementType>
typename ArraySlice<ElementType>::const_iterator constexpr ArraySlice<
    ElementType>::cbegin() const noexcept {
  return begin();
}

template <class ElementType>
typename ArraySlice<ElementType>::const_iterator constexpr ArraySlice<
    ElementType>::cend() const noexcept {
  return end();
}
template <class ElementType>
typename ArraySlice<ElementType>::reverse_iterator constexpr ArraySlice<
    ElementType>::rbegin() const noexcept {
  return reverse_iterator(end());
}

template <class ElementType>
typename ArraySlice<ElementType>::reverse_iterator constexpr ArraySlice<
    ElementType>::rend() const noexcept {
  return reverse_iterator(begin());
}

template <class ElementType>
typename ArraySlice<ElementType>::const_reverse_iterator constexpr ArraySlice<
    ElementType>::crbegin() const noexcept {
  return rbegin();
}

template <class ElementType>
typename ArraySlice<ElementType>::const_reverse_iterator constexpr ArraySlice<
    ElementType>::crend() const noexcept {
  return rend();
}

template <class ElementType>
constexpr bool operator==(const ArraySlice<ElementType>& lhs,
                          const ArraySlice<ElementType>& rhs) noexcept {
  return (std::distance(lhs.begin(), lhs.end()) ==
          std::distance(rhs.begin(), rhs.end())) &&
         std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

template <class ElementType>
constexpr bool operator!=(const ArraySlice<ElementType>& lhs,
                          const ArraySlice<ElementType>& rhs) noexcept {
  return !(rhs == rhs);
}

template <class ElementType>
constexpr bool operator<(const ArraySlice<ElementType>& lhs,
                         const ArraySlice<ElementType>& rhs) noexcept {
  return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(),
                                      rhs.end());
}

template <class ElementType>
constexpr bool operator>(const ArraySlice<ElementType>& lhs,
                         const ArraySlice<ElementType>& rhs) noexcept {
  return rhs < lhs;
}

template <class ElementType>
constexpr bool operator<=(const ArraySlice<ElementType>& lhs,
                          const ArraySlice<ElementType>& rhs) noexcept {
  return !(rhs < lhs);
}

template <class ElementType>
constexpr bool operator>=(const ArraySlice<ElementType>& lhs,
                          const ArraySlice<ElementType>& rhs) noexcept {
  return !(lhs < rhs);
}

}  // namespace base

#endif  // BASE_ARRAY_SLICE_H_
