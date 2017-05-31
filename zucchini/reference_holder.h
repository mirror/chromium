// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ZUCCHINI_REFERENCE_HOLDER_H_
#define ZUCCHINI_REFERENCE_HOLDER_H_

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <utility>
#include <vector>

#include "zucchini/disassembler.h"
#include "zucchini/image_utils.h"
#include "zucchini/ranges/algorithm.h"
#include "zucchini/ranges/iterator_facade.h"
#include "zucchini/ranges/utility.h"
#include "zucchini/ranges/view.h"

namespace zucchini {

constexpr struct SortedByTypeTag {
} SortedByType{};

constexpr struct SortedByLocationTag {
} SortedByLocation{};

// A class to hold references of an image file. References are grouped by type.
// We assume that references do not collide, and within each group, they are
// ordered by |location|. We provide interfaces to iterate over references.
class ReferenceHolder {
  using RefVector = std::vector<std::vector<Reference>>;
  struct LastTag {};
  static const LastTag kLast;

  // A cursor to iterate through references, ordered by their types.
  template <class T, class C>
  class TypeSortedCursor {
    template <class T2, class C2>
    friend class TypeSortedCursor;

   public:
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::forward_iterator_tag;

    TypeSortedCursor() = delete;
    explicit TypeSortedCursor(C* refs);
    TypeSortedCursor(C* refs, LastTag)
        : references_(refs), typeIdx_(references_->size()) {}
    template <class T2>
    TypeSortedCursor(const TypeSortedCursor<T2, C>& that)
        : references_(that.references_),
          typeIdx_(that.typeIdx_),
          refIdx_(that.refIdx_) {}

    void next();
    T get() const;
    template <class T2>
    bool equal(const TypeSortedCursor<T2, C>& b) const;

   private:
    C* references_ = nullptr;
    size_t typeIdx_;
    size_t refIdx_ = 0;
  };

  // A cursor to iterate through references, ordered by their locations. This
  // requires traversing different reference groups in lockstep. We optimize
  // this by storing per-group indexes in a min-|heap_|.
  template <class T, class C>
  class LocSortedCursor {
    template <class T2, class C2>
    friend class LocSortedCursor;

    struct Comp {
      explicit Comp(C& c) : c_(c) {}
      bool operator()(const std::pair<size_t, size_t>& a,
                      const std::pair<size_t, size_t>& b) const;
      C& c_;
    };

   public:
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::forward_iterator_tag;

    LocSortedCursor() = delete;
    explicit LocSortedCursor(C* refs);
    LocSortedCursor(C* refs, LastTag) : references_(refs) {}
    template <class T2>
    explicit LocSortedCursor(LocSortedCursor<T2, C>&& that)
        : references_(std::move(that.references_)),
          heap_(std::move(that.heap_)) {}

    void next();
    T get() const;
    template <class T2>
    bool equal(const LocSortedCursor<T2, C>& b) const;

   private:
    C* references_ = nullptr;

    // Each element in |heap_| stores {first: type, second: index}, representing
    // the Reference instance |references_[type][index]|.
    std::vector<std::pair<size_t, size_t>> heap_;
  };

  template <class Ref,
            class Value,
            template <class, class> class Cursor,
            class C>
  class View {
   public:
    using iterator = ranges::IteratorFacade<Cursor<Ref, C>>;
    using const_iterator = ranges::IteratorFacade<Cursor<Value, C>>;

    explicit View(C* refs) : references_(refs) {}

    iterator begin() const { return iterator{references_}; }
    iterator end() const { return iterator{references_, kLast}; }

   private:
    C* references_;
  };

 public:
  using Range = ranges::View<std::vector<Reference>>;
  using ConstRange = ranges::View<const std::vector<Reference>>;

  using TypeSortedRange = ranges::RngFacade<
      View<MutableTypedReference, TypedReference, TypeSortedCursor, RefVector>>;
  using ConstTypeSortedRange = ranges::RngFacade<
      View<TypedReference, TypedReference, TypeSortedCursor, const RefVector>>;

  using LocSortedRange = ranges::RngFacade<
      View<MutableTypedReference, TypedReference, LocSortedCursor, RefVector>>;
  using ConstLocSortedRange = ranges::RngFacade<
      View<TypedReference, TypedReference, LocSortedCursor, const RefVector>>;

  ReferenceHolder();
  ReferenceHolder(ReferenceHolder&);  // No explicit.
  explicit ReferenceHolder(uint8_t type_count);
  ~ReferenceHolder();

  // Returns an empty instance, to be used as stub.
  static const ReferenceHolder& GetEmptyRefs();

  void Insert(GroupRange refs);
  void Insert(const ReferenceGroup& refs);
  template <class Rng>
  void Insert(const ReferenceTraitsBasic& traits, Rng&& refs);

  Range Get(uint8_t type) { return Range{references_[type]}; }
  ConstRange Get(uint8_t type) const { return ConstRange{references_[type]}; }
  TypeSortedRange Get(SortedByTypeTag) { return TypeSortedRange{&references_}; }
  ConstTypeSortedRange Get(SortedByTypeTag) const {
    return ConstTypeSortedRange{&references_};
  }
  LocSortedRange Get(SortedByLocationTag) {
    return LocSortedRange{&references_};
  }
  ConstLocSortedRange Get(SortedByLocationTag) const {
    return ConstLocSortedRange{&references_};
  }

  size_t TypeCount() const { return references_.size(); }
  size_t PoolCount() const { return pool_count_; }
  offset_t Width(uint8_t type) const { return traits_[type].width; }
  uint8_t Pool(uint8_t type) const { return traits_[type].pool; }

  Reference Find(uint8_t type, offset_t location) const;

 private:
  // For a given type, |references_[type]| stores a list of Reference instances
  // sorted by location (file offset).
  std::vector<std::vector<Reference>> references_;

  std::vector<ReferenceTraitsBasic> traits_;
  size_t pool_count_ = 0;
};

/******** ReferenceHolder::TypeSortedCursor ********/

template <class T, class C>
ReferenceHolder::TypeSortedCursor<T, C>::TypeSortedCursor(C* refs)
    : references_(refs) {
  typeIdx_ = 0;
  while (typeIdx_ < references_->size() &&
         ranges::empty((*references_)[typeIdx_]))
    ++typeIdx_;
}

template <class T, class C>
void ReferenceHolder::TypeSortedCursor<T, C>::next() {
  ++refIdx_;
  while (typeIdx_ < references_->size() &&
         refIdx_ >= ranges::size((*references_)[typeIdx_])) {
    ++typeIdx_;
    refIdx_ = 0;
  }
}

template <class T, class C>
T ReferenceHolder::TypeSortedCursor<T, C>::get() const {
  return {uint8_t(typeIdx_), (*references_)[typeIdx_][refIdx_]};
}

template <class T, class C>
template <class T2>
bool ReferenceHolder::TypeSortedCursor<T, C>::equal(
    const TypeSortedCursor<T2, C>& b) const {
  return typeIdx_ == b.typeIdx_ && refIdx_ == b.refIdx_;
}

/******** ReferenceHolder::LocSortedCursor ********/

template <class T, class C>
ReferenceHolder::LocSortedCursor<T, C>::LocSortedCursor(C* refs)
    : references_(refs) {
  heap_.reserve(references_->size());
  for (size_t i = 0; i < references_->size(); ++i) {
    if (!ranges::empty((*references_)[i])) {
      heap_.push_back(std::pair<size_t, size_t>(i, 0));
    }
  }
  std::make_heap(heap_.begin(), heap_.end(), Comp(*references_));
}

template <class T, class C>
bool ReferenceHolder::LocSortedCursor<T, C>::Comp::operator()(
    const std::pair<size_t, size_t>& a,
    const std::pair<size_t, size_t>& b) const {
  offset_t loc1 = c_[a.first][a.second].location;
  offset_t loc2 = c_[b.first][b.second].location;
  if (loc1 != loc2) {
    return loc1 > loc2;
  }
  return a.first > b.first;
}

template <class T, class C>
void ReferenceHolder::LocSortedCursor<T, C>::next() {
  // Extract the top element of |heap_|, and increment its index |second|.
  std::pop_heap(heap_.begin(), heap_.end(), Comp(*references_));
  ++heap_.back().second;
  // If the element reached the final for its given type |first|, remove it.
  // Otherwise reinstate it into |heap_| for the selection of a new top element.
  if (heap_.back().second == ranges::size((*references_)[heap_.back().first]))
    heap_.pop_back();
  else
    std::push_heap(heap_.begin(), heap_.end(), Comp(*references_));
}

template <class T, class C>
T ReferenceHolder::LocSortedCursor<T, C>::get() const {
  return {uint8_t(heap_[0].first),
          (*references_)[heap_[0].first][heap_[0].second]};
}

template <class T, class C>
template <class T2>
bool ReferenceHolder::LocSortedCursor<T, C>::equal(
    const LocSortedCursor<T2, C>& b) const {
  if (heap_.empty() || b.heap_.empty()) {
    return heap_.empty() == b.heap_.empty();
  }
  return heap_[0] == b.heap_[0];
}

/******** ReferenceHolder ********/

template <class Rng>
void ReferenceHolder::Insert(const ReferenceTraitsBasic& traits, Rng&& refs) {
  if (traits.type >= references_.size()) {
    // O(n^2) worst case, but these vectors are small so this is okay.
    references_.resize(traits.type + 1);
    traits_.resize(traits.type + 1);
  }
  traits_[traits.type] = traits;
  pool_count_ = std::max(pool_count_, size_t(traits.pool + 1));

  ranges::copy(refs, std::back_inserter(references_[traits.type]));
}

}  // namespace zucchini

#endif  // ZUCCHINI_REFERENCE_HOLDER_H_
