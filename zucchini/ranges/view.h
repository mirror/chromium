// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ZUCCHINI_RANGES_VIEW_H_
#define ZUCCHINI_RANGES_VIEW_H_

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <utility>

#include "zucchini/ranges/functional.h"
#include "zucchini/ranges/iterator_facade.h"
#include "zucchini/ranges/range_facade.h"
#include "zucchini/ranges/utility.h"

namespace zucchini {
namespace ranges {
namespace view {

template <class T>
class IotaView {
  class Cursor : public BaseCursor<Cursor> {
   public:
    using iterator_category = std::random_access_iterator_tag;

    Cursor() = default;
    explicit Cursor(T idx) : idx_(idx) {}

    T get() const { return idx_; }
    void advance(std::ptrdiff_t n) { idx_ += n; }
    std::ptrdiff_t distance(const Cursor& b) const { return idx_ - b.idx_; }

   private:
    T idx_ = 0;
  };

 public:
  using iterator = IteratorFacade<Cursor>;

  IotaView(T first, T last) : first_(first), last_(last) {}

  iterator begin() const { return iterator{first_}; }
  iterator end() const { return iterator{last_}; }

 private:
  T first_ = 0;
  T last_ = 0;
};

template <class T>
RngFacade<IotaView<T>> Iota(T first, T last) {
  return RngFacade<IotaView<T>>{first, last};
}

template <class Rng>
class DropView : public range_traits<Rng> {
 public:
  using iterator = typename range_traits<Rng>::iterator;
  using sentinel = typename range_traits<Rng>::sentinel;

  template <class Difference>
  DropView(Rng&& rng, Difference n)
      : first_(std::next(std::begin(rng), n)), last_(std::end(rng)) {}
  DropView(iterator first, sentinel last) : first_(first), last_(last) {}

  iterator begin() const { return first_; }
  sentinel end() const { return last_; }

 private:
  iterator first_;
  sentinel last_;
};

template <class Rng, class Difference>
RngFacade<DropView<Rng>> Drop(Rng&& rng, Difference from) {
  return RngFacade<DropView<Rng>>{std::forward<Rng>(rng), from};
}

template <class Rng, class UnaryPredicate>
class RemoveIfView : public BaseView<Rng> {
 public:
  using iterator = typename BaseView<Rng>::iterator;

  class Adaptor : public BaseAdaptor {
    const RemoveIfView* view_;

   public:
    using iterator_category = std::bidirectional_iterator_tag;

    explicit Adaptor(const RemoveIfView& view) : view_(&view) {}

    template <class It>
    void next(It& it) const {
      ++it;
      while (it != view_->end() && (view_->pred_)(*it))
        ++it;
    }
    template <class It>
    void prev(It& it) const {  // NOLINT for It&.
      --it;
      while (it != view_->begin() && (view_->pred_)(*it))
        --it;
    }
    template <class It, class Difference>
    void advance(Difference n) = delete;
    template <class It1, class It2>
    void distance(It1 a, It2 b) = delete;
  };

  Adaptor adaptor() const { return Adaptor(*this); }
  iterator begin() const { return first_; }

  RemoveIfView(Rng&& rng, UnaryPredicate pred)
      : BaseView<Rng>(std::forward<Rng>(rng)),
        pred_(std::move(pred)),
        first_(std::begin(this->rng_)) {
    while (first_ != std::end(this->rng_) && pred_(*first_))
      ++first_;
  }

 private:
  UnaryPredicate pred_;
  iterator first_;
};

template <class Rng, class UnaryPred>
RngAdaptor<RemoveIfView<Rng, UnaryPred>> RemoveIf(Rng&& rng, UnaryPred op) {
  return RngAdaptor<RemoveIfView<Rng, UnaryPred>>{std::forward<Rng>(rng),
                                                  std::move(op)};
}

template <class Rng, class UnaryOp>
class TransformView : public BaseView<Rng> {
  UnaryOp op_;

 public:
  class Adaptor : public BaseAdaptor {
    const UnaryOp* op_;

   public:
    Adaptor(const UnaryOp& op) : op_(&op) {}  // NOLINT for no explicit.

    template <class Iterator>
    auto get(Iterator& it) const -> decltype((*op_)(*it)) {
      return (*op_)(*it);
    }
    template <class Iterator, class Difference>
    auto at(Iterator& it, Difference n) const  // NOLINT for Iterator&.
        -> decltype((*op_)(it[n])) {
      return (*op_)(it[n]);
    }
  };

  Adaptor adaptor() const { return {op_}; }

  TransformView(Rng&& rng, UnaryOp op)
      : BaseView<Rng>(std::forward<Rng>(rng)), op_(std::move(op)) {}
};

template <class Rng, class UnaryOp, class Proj = as_function_t<UnaryOp>>
RngAdaptor<TransformView<Rng, Proj>> Transform(Rng&& rng, UnaryOp op) {
  return RngAdaptor<TransformView<Rng, Proj>>{std::forward<Rng>(rng),
                                              as_function(std::move(op))};
}

template <class Rng>
RngAdaptor<TransformView<Rng, key>> Keys(Rng&& rng) {
  return RngAdaptor<TransformView<Rng, key>>{std::forward<Rng>(rng), key()};
}

template <class Gen, class Value>
class IterableGeneratorView {
  class Cursor : public BaseCursor<Cursor> {
    Gen gen_;
    Value value_;
    bool done_;

   public:
    using iterator_category = std::forward_iterator_tag;

    explicit Cursor(const Gen& gen) : gen_(gen) { next(); }

    const Value& get() { return value_; }
    void next() { done_ = !gen_(&value_); }
    bool equal(const BaseSentinel& that) const { return done_; }
    bool equal(const Cursor& that) const { return false; }
  };

 public:
  using iterator = IteratorFacade<Cursor>;
  using sentinel = IteratorFacade<BaseSentinel>;
  using const_iterator = IteratorFacade<Cursor>;
  using const_sentinel = IteratorFacade<BaseSentinel>;

  explicit IterableGeneratorView(const Gen& gen) : gen_(gen) {}

  iterator begin() const { return iterator{gen_}; }
  sentinel end() const { return sentinel{}; }

 private:
  Gen gen_;
};

template <class Value, class Gen>
using Iterable = RngFacade<IterableGeneratorView<Gen, Value>>;

template <class Value, class Gen>
Iterable<Value, Gen> MakeIterable(const Gen& gen) {
  return Iterable<Value, Gen>{gen};
}

template <class Value, class Gen>
Iterable<Value, std::function<bool(Value*)>> MakePtrIterable(Gen* gen) {
  auto gen_wrap = [gen](Value* v) -> bool { return (*gen)(v); };
  return Iterable<Value, std::function<bool(Value*)>>{gen_wrap};
}

}  // namespace view
}  // namespace ranges
}  // namespace zucchini

#endif  // ZUCCHINI_RANGES_VIEW_H_
