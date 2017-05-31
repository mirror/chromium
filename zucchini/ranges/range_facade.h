// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ZUCCHINI_RANGES_RANGE_FACADE_H_
#define ZUCCHINI_RANGES_RANGE_FACADE_H_

#include <cstddef>
#include <iterator>
#include <utility>

#include "zucchini/ranges/iterator_facade.h"
#include "zucchini/ranges/utility.h"

namespace zucchini {
namespace ranges {

template <class Adaptor, class Iterator>
class CursorAdaptor {
  template <class Adaptor2, class Iterator2>
  friend class CursorAdaptor;

 public:
  using difference_type =
      typename std::iterator_traits<Iterator>::difference_type;
  using iterator_category = typename std::common_type<
      typename std::iterator_traits<Iterator>::iterator_category,
      typename Adaptor::iterator_category>::type;

  CursorAdaptor(const Adaptor& ad, const Iterator& it) : ad_(ad), it_(it) {}
  template <class It2>
  CursorAdaptor(const CursorAdaptor<Adaptor, It2>& that)
      : ad_(that.ad_), it_(that.it_) {}
  template <class It2>
  CursorAdaptor(CursorAdaptor<Adaptor, It2>&& that)
      : ad_(std::move(that.ad_)), it_(std::move(that.it_)) {}
  template <class It2>
  CursorAdaptor& operator=(const CursorAdaptor<Adaptor, It2>& that) {
    ad_ = that.ad_;
    it_ = that.it_;
    return *this;
  }
  template <class It2>
  CursorAdaptor& operator=(CursorAdaptor<Adaptor, It2>&& that) {
    ad_ = std::move(that.ad_);
    it_ = std::move(that.it_);
    return *this;
  }

  auto get()
      -> decltype(std::declval<Adaptor&>().get(std::declval<Iterator&>())) {
    return ad_.get(it_);
  }
  auto at(std::ptrdiff_t n)
      -> decltype(std::declval<Adaptor&>().at(std::declval<Iterator&>(), n)) {
    return ad_.at(it_, n);
  }

  void next() { ad_.next(it_); }
  void prev() { ad_.prev(it_); }
  void advance(difference_type n) { ad_.advance(it_, n); }
  template <class It2>
  difference_type distance(const CursorAdaptor<Adaptor, It2>& b) const {
    return ad_.distance(it_, b.it_);
  }
  template <class It2>
  bool equal(const CursorAdaptor<Adaptor, It2>& b) const {
    return ad_.equal(it_, b.it_);
  }
  template <class It2>
  bool less(const CursorAdaptor<Adaptor, It2>& b) const {
    return ad_.less(it_, b.it_);
  }

 private:
  Adaptor ad_;
  Iterator it_;
};

template <class Adaptor, class Iterator>
using IteratorAdaptor = IteratorFacade<CursorAdaptor<Adaptor, Iterator>>;

class BaseAdaptor {
 public:
  using iterator_category = std::random_access_iterator_tag;

  template <class Iterator>
  auto get(Iterator& it) const -> decltype(*it) {  // NOLINT for Iterator&.
    return *it;
  }
  template <class Iterator>
  auto at(Iterator& it, std::ptrdiff_t n) const -> decltype(it[n]) {  // NOLINT.
    return it[n];
  }

  template <class Iterator>
  void next(Iterator& it) const {  // NOLINT.
    ++it;
  }
  template <class Iterator>
  void prev(Iterator& it) const {  // NOLINT.
    --it;
  }
  template <class Iterator, class Difference>
  void advance(Iterator& it, Difference n) const {  // NOLINT.
    it += n;
  }

  template <class Iterator1, class Iterator2>
  auto distance(const Iterator1& a, const Iterator2& b) const
      -> decltype(a - b) {
    return a - b;
  }

  template <class Iterator1, class Iterator2>
  bool equal(const Iterator1& a, const Iterator2& b) const {
    return a == b;
  }
  template <class Iterator1, class Iterator2>
  bool less(const Iterator1& a, const Iterator2& b) const {
    return a < b;
  }
};

template <class Rng>
class BaseView : range_traits<Rng> {
 public:
  using size_type = typename range_traits<Rng>::size_type;
  using iterator = typename range_traits<Rng>::iterator;
  using sentinel = typename range_traits<Rng>::sentinel;

  explicit BaseView(Rng&& rng) : rng_(std::forward<Rng>(rng)) {}

  iterator begin() const { return std::begin(rng_); }
  sentinel end() const { return std::end(rng_); }

 protected:
  Rng&& rng_;
};

// Helper class to create ranges.
//
// To create a range, provide a core class which
// defines the following when it make sense :
//  iterator (optional)
//  sentinel (optional).
//  const_iterator (optional).
//  const_sentinel (optional).
//  begin()  (optional).
//  end()  (optional).
template <class Core>
class RngFacade {
  struct DefaultedSentinel {
    template <class U>
    static typename U::sentinel test();
    template <class U, class... E>
    static typename U::iterator test(E...);
    using type = decltype(test<Core>());
  };

  struct DefaultedConstIterator {
    template <class U>
    static typename U::const_iterator test();
    template <class U, class... E>
    static typename U::iterator test(E...);
    using type = decltype(test<Core>());
  };

  struct DefaultedConstSentinel {
    template <class U>
    static typename U::const_sentinel test();
    template <class U, class... E>
    static typename DefaultedConstIterator::type test(E...);
    using type = decltype(test<Core>());
  };

 public:
  using iterator = typename Core::iterator;
  using sentinel = typename DefaultedSentinel::type;
  using const_iterator = typename DefaultedConstIterator::type;
  using const_sentinel = typename DefaultedConstSentinel::type;

  using size_type = typename DefaultedSizeType<Core>::type;

  template <class... Args>
  RngFacade(Args&&... args) : core_(std::forward<Args>(args)...) {}  // NOLINT.
  template <class C2>
  RngFacade(RngFacade<C2>& that) : core_(that.core_) {}
  template <class C2>
  RngFacade(const RngFacade<C2>& that) : core_(that.core_) {}
  template <class C2>
  RngFacade(RngFacade<C2>&& that) : core_(std::move(that.core_)) {}
  template <class C2>
  RngFacade& operator=(const RngFacade<C2>& that) {
    core_ = that.core_;
    return *this;
  }
  template <class C2>
  RngFacade& operator=(RngFacade<C2>&& that) {
    core_ = std::move(that.core_);
    return *this;
  }

  template <class T = Core, class U = decltype(std::declval<T>().size())>
  size_type size() const {
    return core_.size();
  }

  iterator begin() const { return core_.begin(); }
  sentinel end() const { return core_.end(); }
  const_iterator cbegin() const { return core_.begin(); }
  const_sentinel cend() const { return core_.end(); }

 protected:
  Core core_;
};

// Helper class to create range adaptors.
//
// To create a range adaptor, provide a core class which
// defines the following when it make sense :
//  iterator (optional)
//  sentinel (optional).
//  const_iterator (optional).
//  const_sentinel (optional).
//  begin()  (optional).
//  end()  (optional).
//  adaptor()
template <class Core>
class RngAdaptor {
  using Adaptor = decltype(std::declval<Core>().adaptor());

 public:
  using iterator = IteratorAdaptor<Adaptor, typename RngFacade<Core>::iterator>;
  using sentinel = IteratorAdaptor<Adaptor, typename RngFacade<Core>::sentinel>;
  using const_iterator =
      IteratorAdaptor<Adaptor, typename RngFacade<Core>::const_iterator>;
  using const_sentinel =
      IteratorAdaptor<Adaptor, typename RngFacade<Core>::const_sentinel>;

  using size_type = typename DefaultedSizeType<Core>::type;

  template <class... Args>
  RngAdaptor(Args&&... args) : core_(std::forward<Args>(args)...) {}  // NOLINT.
  template <class C2>
  RngAdaptor(RngAdaptor<C2>& that) : core_(that.core_) {}
  template <class C2>
  RngAdaptor(const RngAdaptor<C2>& that) : core_(that.core_) {}
  template <class C2>
  RngAdaptor(RngAdaptor<C2>&& that) : core_(std::move(that.core_)) {}
  template <class C2>
  RngAdaptor& operator=(const RngAdaptor<C2>& that) {
    core_ = that.core_;
    return *this;
  }
  template <class C2>
  RngAdaptor& operator=(RngAdaptor<C2>&& that) {
    core_ = std::move(that.core_);
    return *this;
  }

  template <class T = Core, class U = decltype(std::declval<T>().size())>
  size_type size() const {
    return core_.size();
  }

  iterator begin() const { return iterator{core_.adaptor(), core_.begin()}; }
  sentinel end() const { return sentinel{core_.adaptor(), core_.end()}; }
  const_iterator cbegin() const {
    return const_iterator{core_.adaptor(), core_.begin()};
  }
  const_sentinel cend() const {
    return const_sentinel{core_.adaptor(), core_.end()};
  }

 private:
  Core core_;
};

// Adapts a Rng to hide all methods but those related to the Iterable concept,
// that is : begin(), end(), cbegin(), cend(), size()
template <class Rng>
using View = RngFacade<BaseView<Rng&>>;

}  // namespace ranges
}  // namespace zucchini

#endif  // ZUCCHINI_RANGES_RANGE_FACADE_H_
