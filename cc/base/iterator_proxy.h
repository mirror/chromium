// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_BASE_ITERATOR_PROXY_H_
#define CC_BASE_ITERATOR_PROXY_H_

#include "base/optional.h"
#include "base/template_util.h"

namespace cc {
namespace internal {

template <typename T, typename = void>
struct ArrowReturnType : std::false_type {
  using type = void;
};

template <typename T>
struct ArrowReturnType<T,
                       base::void_t<decltype(std::declval<T>().operator->())>>
    : std::true_type {
  using type = decltype(std::declval<T>().operator->());
};

template <typename T>
using HasArrowAccessor = ArrowReturnType<T>;

}  // namespace internal

// This template provides a generic conversion from Java-style iterators to
// C++-style iterators. That is given an iterator Iter that has operator++,
// operator*, operator->, and operator bool, this generates a
// IteratorProxy<Iter> that can be used as follows:
//
// IteratorProxy<Iter> begin(iter);
// IteratorProxy<Iter> end;
// for (auto it = begin; it != end; ++it)
//   ...
//
// This is useful to convert existing Java-style iterators to be compatable with
// range for loops.
template <typename Iterator>
class IteratorProxy {
 private:
  base::Optional<Iterator> iterator_;

 public:
  // Provides a generic "end" iterator proxy sentinel.
  IteratorProxy() = default;

  // Provides parameters to construct an iterator to iterate.
  template <typename... Args>
  explicit IteratorProxy(Args... args)
      : iterator_(base::in_place_t(), std::forward<Args>(args)...) {}

  // This is a copy constructor. It needs to be a templated type so that we can
  // generated only if the underlying Iterator is copy constructible. We also
  // ensure that whatever the deduced type is here, it is the same as Iterator.
  // Essentially, this ensures that this is enabled only if IteratorCopy ==
  // Iterator and Iterator is copy constructible.
  template <typename IteratorCopy,
            typename std::enable_if<
                std::is_same<Iterator,
                             typename std::decay<IteratorCopy>::type>::value &&
                    std::is_copy_constructible<IteratorCopy>::value,
                void*>::type = nullptr>
  explicit IteratorProxy(const IteratorCopy& other)
      : iterator_(other.iterator_) {}

  IteratorProxy(IteratorProxy&& other) = default;

  IteratorProxy& operator++() {
    DCHECK(iterator_ && *iterator_);
    ++*iterator_;
    return *this;
  }
  IteratorProxy& operator++(int) {
    IteratorProxy copy = *this;
    ++(*this);
    return copy;
  }
  auto operator*() const -> decltype(**iterator_) {
    DCHECK(iterator_ && *iterator_);
    return **iterator_;
  }

  // Make sure this operator is a template, so that it isn't instantiated if
  // it's never called. In order to write the return type, we need to SFINAE on
  // whether the iterator has the operator (and add enable_if so that we get a
  // nice error when this fails).
  template <
      typename ReturnType = typename internal::ArrowReturnType<Iterator>::type>
  auto operator-> () const ->
      typename std::enable_if<internal::HasArrowAccessor<Iterator>::value,
                              ReturnType>::type {
    DCHECK(iterator_ && *iterator_);
    return iterator_->operator->();
  }

  // Note this should only be used to compare to "end", aka default constructed
  // IteratorProxy or an IteratorProxy that has reached the "end" element, since
  // it cannot determine otherwise if the iterators are equal.
  bool operator==(const IteratorProxy& other) {
    DCHECK(!iterator_ || !other.iterator_);
    return (!iterator_ || !*iterator_) &&
           (!other.iterator_ || !*other.iterator_);
  }

  // Note this should only be used to compare to "end", aka default constructed
  // IteratorProxy or an IteratorProxy that has reached the "end" element, since
  // it cannot determine otherwise if the iterators are equal.
  bool operator!=(const IteratorProxy& other) { return !(*this == other); }
};

}  // namespace cc

#endif  // CC_BASE_ITERATOR_PROXY_H_
