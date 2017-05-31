// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ZUCCHINI_RANGES_ITERATOR_FACADE_H_
#define ZUCCHINI_RANGES_ITERATOR_FACADE_H_

#include <cstddef>
#include <type_traits>
#include <utility>

#include "zucchini/ranges/meta.h"
#include "zucchini/ranges/utility.h"

namespace zucchini {
namespace ranges {

// Allows rvalues to by used with operator -> ()
template <class T>
struct ProxyPtr {
  T value;
  T* operator->() { return &value; }
};

// Helper class to create iterators.
//
// To create an iterator, provide a core class which
// defines the following when it make sense :
//  iterator_category : typedef using standard tags.
//  get() -> Reference : Dereference the iterator.
//  at(difference_type n) : Access the iterator with an offset of n (optional).
//  advance(difference_type n) : Advance the iterator by n (n can be negative).
//  next() : Increment the iterator (optional if has advance).
//  prev() : Decrement the iterator (optional if has advance).
//  distance(Core that) : Return the distance from that to this.
//  equal(Core that) : Return true if equal to that (optional if has distance).
//  less(Core that) : Return true if less than that (optional if has distance).
template <class Core>
class IteratorFacade {
  template <class C2>
  friend class IteratorFacade;

  using Reference = decltype(std::declval<Core&>().get());
  using Value = typename std::remove_reference<Reference>::type;
  using Pointer = ProxyPtr<Reference>;

 public:
  using reference = Reference;
  using value_type = Value;
  using pointer = Pointer;
  using difference_type = typename Core::difference_type;
  using iterator_category = typename Core::iterator_category;

  template <class... Args>
  IteratorFacade(Args&&... args)  // NOLINT.
      : core_(std::forward<Args>(args)...) {}
  template <class C2>
  IteratorFacade(IteratorFacade<C2>& that) : core_(that.core_) {}
  template <class C2>
  IteratorFacade(const IteratorFacade<C2>& that) : core_(that.core_) {}
  template <class C2>
  IteratorFacade(IteratorFacade<C2>&& that) : core_(std::move(that.core_)) {}
  template <class C2>
  IteratorFacade& operator=(const IteratorFacade<C2>& that) {
    core_ = that.core_;
    return *this;
  }
  template <class C2>
  IteratorFacade& operator=(IteratorFacade<C2>&& that) {
    core_ = std::move(that.core_);
    return *this;
  }

  reference operator*() {
    CheckRet<reference>();
    return core_.get();
  }
  pointer operator->() { return {core_.get()}; }
  reference operator[](difference_type n) {
    CheckRet<reference>();
    return core_.at(n);
  }

  IteratorFacade& operator++() {
    core_.next();
    return *this;
  }
  IteratorFacade operator++(int) {
    auto that = *this;
    core_.next();
    return that;
  }
  IteratorFacade& operator--() {
    core_.prev();
    return *this;
  }
  IteratorFacade operator--(int) {
    auto that = *this;
    core_.prev();
    return that;
  }
  IteratorFacade& operator+=(difference_type n) {
    core_.advance(n);
    return *this;
  }
  IteratorFacade& operator-=(difference_type n) {
    core_.advance(-n);
    return *this;
  }
  IteratorFacade operator+(difference_type n) const {
    IteratorFacade that = *this;
    that.core_.advance(n);
    return that;
  }
  IteratorFacade operator-(difference_type n) const {
    auto that = *this;
    that.core_.advance(-n);
    return that;
  }

  template <class C2>
  difference_type operator-(const IteratorFacade<C2>& that) const {
    return core_.distance(that.core_);
  }

  template <class C2>
  bool operator==(const IteratorFacade<C2>& that) const {
    return core_.equal(that.core_);
  }

  template <class C2>
  bool operator<(const IteratorFacade<C2>& that) const {
    return core_.less(that.core_);
  }

 private:
  Core core_;
};

// Comparison operators
template <class C1, class C2>
bool operator!=(IteratorFacade<C1> const& lhs, IteratorFacade<C2> const& rhs) {
  return !(lhs == rhs);
}

template <class C1, class C2>
bool operator<=(IteratorFacade<C1> const& lhs, IteratorFacade<C2> const& rhs) {
  return !(rhs < lhs);
}

template <class C1, class C2>
bool operator>(IteratorFacade<C1> const& lhs, IteratorFacade<C2> const& rhs) {
  return rhs < lhs;
}

template <class C1, class C2>
bool operator>=(IteratorFacade<C1> const& lhs, IteratorFacade<C2> const& rhs) {
  return !(lhs < rhs);
}

// Iterator addition
template <class C>
IteratorFacade<C> operator+(typename IteratorFacade<C>::difference_type n,
                            const IteratorFacade<C>& it) {
  return it + n;
}

// Helper class to create cursor core.
//
// Derives the cursor core from BaseCursor to benefit from the default
// definition of :
//  difference_type = std::ptrdiff_t
//  at(), next(), prev(), less(), equal()
template <class Derived>
class BaseCursor {
 public:
  using difference_type = std::ptrdiff_t;

  template <class T = Derived>
  auto at(difference_type n) -> decltype(std::declval<T&>().get()) {
    T that = derived();
    that.derived().advance(n);
    return that.derived().get();
  }

  void next() { derived().advance(1); }

  void prev() { derived().advance(-1); }

  template <class C2>
  bool equal(const C2& that) const {
    return derived().distance(that) == 0;
  }

  template <class C2>
  bool less(const C2& that) const {
    return derived().distance(that) < 0;
  }

 private:
  Derived& derived() { return *static_cast<Derived*>(this); }
  const Derived& derived() const { return *static_cast<const Derived*>(this); }
};

// Helper class to create sentinel core.
//
// Derives the sentinel core from BaseSentinel to benefit from the default
// definition of :
//  difference_type = std::ptrdiff_t
//  iterator_category = std::forward_iterator_tag
//  get(), equal()
class BaseSentinel {
 public:
  using difference_type = std::ptrdiff_t;
  using iterator_category = std::forward_iterator_tag;

  void get() {}
  template <class Cursor>
  bool equal(const Cursor& that) const {
    return that.equal(*this);
  }
};

}  // namespace ranges
}  // namespace zucchini

#endif  // ZUCCHINI_RANGES_ITERATOR_FACADE_H_
