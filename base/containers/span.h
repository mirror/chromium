// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_SPAN_H_
#define BASE_SPAN_H_

#include <stddef.h>

#include <algorithm>
#include <type_traits>
#include <utility>

namespace base {

template <typename T>
class Span;

namespace internal {

template <typename T>
struct IsSpanImpl : std::false_type {};

template <typename T>
struct IsSpanImpl<Span<T>> : std::true_type {};

template <typename T>
using IsSpan = IsSpanImpl<std::decay_t<T>>;

template <typename U, typename T>
using IsLegalSpanConversion =
    std::integral_constant<bool, std::is_convertible<U*, T*>::value>;

template <typename C, typename T>
using ContainerHasConvertibleData = IsLegalSpanConversion<
    std::remove_pointer_t<decltype(std::declval<C>().data())>,
    T>;
template <typename C>
using ContainerHasIntegralSize =
    std::is_integral<decltype(std::declval<C>().size())>;

template <typename U, typename T>
using EnableIfLegalSpanConversion =
    std::enable_if_t<IsLegalSpanConversion<U, T>::value>;
// The proposal also ZZ
template <typename C, typename T>
using EnableIfSpanCompatibleContainer =
    std::enable_if_t<!internal::IsSpan<C>::value &&
                     ContainerHasConvertibleData<C, T>::value &&
                     ContainerHasIntegralSize<C>::value>;

}  // namespace internal

// A Span represents an array of elements of type T. It consists of a pointer to
// memory with an associated size. A Span does not own the underlying memory, so
// care must be taken to ensure that a Span does not outlive the backing store.
// Spans should be passed by value.
//
// Span is somewhat analogous to StringPiece, but with arbitrary element types,
// allowing mutation if T is non-const.
//
// Span differs from the C++ working group proposal in a number of ways:
// - Span does not define the |element_type| and |index_type| type aliases.
// - Span does not define operator().
// - Span does not define bytes(), size_bytes(), as_bytes(), as_mutable_bytes()
//   for working with spans as a sequence of bytes.
// - Span has no extent template parameter.
// - Span has no conversion constructors from std::unique_ptr or
//   std::shared_ptr.
// - Span has no reverse iterators.
// - Span does not define relation operators other than == and !=.
//
// TODO(https://crbug.com/754077): Document differences from the working group
// proposal: http://open-std.org/JTC1/SC22/WG21/docs/papers/2016/p0122r1.pdf.
// TODO(https://crbug.com/754077): Implement more Span support, such as
// initialization from containers, and document why this is useful (greater
// safety since no need to manually pass in data + size)
template <typename T>
class Span {
 private:
 public:
  using value_type = T;
  using pointer = T*;
  using reference = T&;
  using iterator = T*;
  using const_iterator = const T*;
  // TODO(dcheng): What about reverse iterators?

  // Span constructors, copy, assignment, and destructor
  constexpr Span() noexcept : data_(nullptr), size_(0) {}
  constexpr Span(T* data, size_t size) noexcept : data_(data), size_(size) {}
  template <size_t N>
  constexpr Span(T (&array)[N]) noexcept : Span(array, N) {}
  // Conversion from a container that provides |T* data()| and |integral_type
  // size()|.
  // TODO(dcheng): Should this be explicit for mutable spans?
  template <typename C,
            typename = internal::EnableIfSpanCompatibleContainer<C, T>>
  constexpr Span(C& container) : Span(container.data(), container.size()) {}
  // Disallow conversion from a container going out of scope, since the
  // resulting span will just point to dangling memory.
  template <typename C,
            typename = internal::EnableIfSpanCompatibleContainer<C, T>>
  Span(const C&&) = delete;
  constexpr Span(const Span&) noexcept = default;
  constexpr Span(Span&&) noexcept = default;
  ~Span() noexcept = default;
  // Conversions from spans of compatible types: this allows a Span<T> to be
  // seamlessly used as a Span<const T>, but not the other way around.
  template <typename U, typename = internal::EnableIfLegalSpanConversion<U, T>>
  constexpr Span(const Span<U>& other) : Span(other.data(), other.size()) {}
  template <typename U, typename = internal::EnableIfLegalSpanConversion<U, T>>
  constexpr Span(Span<U>&& other) : Span(other.data(), other.size()) {}
  constexpr Span& operator=(const Span&) noexcept = default;
  constexpr Span& operator=(Span&&) noexcept = default;

  // Span subviews
  constexpr Span subspan(size_t pos, size_t count) const {
    // Note: ideally this would DCHECK, but it requires fairly horrible
    // contortions.
    return Span(data_ + pos, count);
  }

  // Span observers
  constexpr size_t size() const noexcept { return size_; }

  // Span element access
  constexpr T& operator[](size_t index) const noexcept { return data_[index]; }
  constexpr T* data() const noexcept { return data_; }

  // Span iterator support
  iterator begin() const noexcept { return data_; }
  iterator end() const noexcept { return data_ + size_; }

  const_iterator cbegin() const noexcept { return begin(); }
  const_iterator cend() const noexcept { return end(); }

 private:
  T* data_;
  size_t size_;
};

// Relational operators. Equality is a element-wise comparison.
template <typename T>
constexpr bool operator==(const Span<T>& lhs, const Span<T>& rhs) noexcept {
  return std::equal(lhs.cbegin(), lhs.cend(), rhs.cbegin(), rhs.cend());
}

template <typename T>
constexpr bool operator!=(const Span<T>& lhs, const Span<T>& rhs) noexcept {
  return !(lhs == rhs);
}

// TODO(dcheng): Implement other relational operators.

// Type-deducing helpers for constructing a Span.
template <typename T>
constexpr Span<T> MakeSpan(T* data, size_t size) noexcept {
  return Span<T>(data, size);
}

template <typename T, size_t N>
constexpr Span<T> MakeSpan(T (&array)[N]) noexcept {
  return Span<T>(array);
}

template <typename C,
          typename T = typename C::value_type,
          typename = internal::EnableIfSpanCompatibleContainer<C, T>>
constexpr Span<T> MakeSpan(C& container) {
  return Span<T>(container);
}

}  // namespace base

#endif  // BASE_SPAN_H_
