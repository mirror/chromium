// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ZUCCHINI_EQUIVALENCE_MAP_H_
#define ZUCCHINI_EQUIVALENCE_MAP_H_

#include <algorithm>
#include <vector>

#include "base/macros.h"
#include "zucchini/encoded_view.h"
#include "zucchini/image_utils.h"
#include "zucchini/ranges/algorithm.h"
#include "zucchini/suffix_array.h"

namespace zucchini {

constexpr int kBaseEquivalenceCost = 12;
constexpr int kMinMatchLength = 6;

// An Equivalence is a block of length |length| that approximately match in
// |old_view| at an offset of |src| and in |new_view| at an offset of |dst|.
struct Equivalence {
  // Uses delta-encoded parameters to advance to the next Equivalence.
  void Advance(int32_t diff_src, uint32_t diff_dst, uint32_t new_length) {
    src += static_cast<int32_t>(length + diff_src);
    dst += length + diff_dst;
    length = new_length;
  }

  Equivalence DisplaceBy(offset_t src_base, offset_t dst_base) {
    return {src_base + src, dst_base + dst, length};
  }

  offset_t src;
  offset_t dst;
  offset_t length;
};

constexpr Equivalence kNullEquivalence = {kNullOffset, kNullOffset,
                                          kNullOffset};

// Container of Equivalences between |old_view| and |new_view|.
// Provides utility to project a file offset from one file to another.
class EquivalenceMap {
 public:
  // Functor that maps a file offset within |old_view| to an existing
  // Equivalence. This functor holds iterators within its associated
  // EquivalenceMap. It will be invalidated if the EquivalenceMap is modified.
  //
  // For performance motivation, the following restrictions are made:
  // The associated EquivalenceMap must have its Equivalences sorted by Source
  // when this functor is being used. The call operator shall be used only with
  // file offsets that are in ascending order.
  class ForwardMapper {
   public:
    using iterator = std::vector<Equivalence>::const_iterator;

    ForwardMapper(const ForwardMapper&);
    ~ForwardMapper();

    iterator operator()(offset_t src);

   private:
    friend class EquivalenceMap;
    ForwardMapper(iterator first, iterator last);

    iterator first_;
    iterator last_;
  };

  // Functor that maps a file offset within |new_view| to an existing
  // Equivalence. This functor holds iterators within its associated
  // EquivalenceMap. It will be invalidated if the EquivalenceMap is modified.
  //
  // For performance motivation, the following restrictions are made:
  // The associated EquivalenceMap must have its Equivalences sorted by
  // Destination when this functor is being used. The call operator shall be
  // used only with file offsets that are in ascending order.
  class BackwardMapper {
    using iterator = std::vector<Equivalence>::const_iterator;

   public:
    BackwardMapper(const BackwardMapper&);
    ~BackwardMapper();

    iterator operator()(offset_t dst);

   private:
    friend class EquivalenceMap;
    BackwardMapper(iterator first, iterator last);

    iterator first_;
    iterator last_;
  };

  using iterator = std::vector<Equivalence>::iterator;
  using const_iterator = std::vector<Equivalence>::const_iterator;

  EquivalenceMap();
  ~EquivalenceMap();

  // Finds relevant Equivalences between |old_view| and |new_view|, using suffix
  // array |old_sa| computed fron |old_view|. This function is not symmetric.
  // Equivalences might overlap in |old_view|, but not in |new_view|. It tries
  // to minimize the accumulated distance within each Equivalence using
  // |distance| callable, while maximizing |new_view| coverage.
  //
  // The minimum length of an Equivalence is given by |minimum_length|.
  // Equivalences will be sorted by Source.
  void Build(const EncodedView& old_view,
             const SuffixArray<const EncodedView&>& old_sa,
             const EncodedView& new_view,
             int minimum_length);

  // Returns an instance of ForwardMapper associated with this object. Must be
  // sorted by Source when called.
  ForwardMapper GetForwardMapper() const { return {begin(), end()}; }

  // Returns an instance of BackwardMapper associated with this object. Must be
  // sorted by Destination when called.
  BackwardMapper GetBackwardMapper() const { return {begin(), end()}; }

  // Sorts Equivalences by ascending order according to their |src|.
  void SortBySource() {
    ranges::sort(equivs_, ranges::less(), &Equivalence::src);
  }

  // Sorts Equivalences by ascending order according to their |dst|.
  void SortByDestination() {
    ranges::sort(equivs_, ranges::less(), &Equivalence::dst);
  }

  size_t size() const { return equivs_.size(); }

  iterator begin() { return equivs_.begin(); }
  iterator end() { return equivs_.end(); }
  const_iterator begin() const { return equivs_.begin(); }
  const_iterator end() const { return equivs_.end(); }

 private:
  std::vector<Equivalence> equivs_;

  DISALLOW_COPY_AND_ASSIGN(EquivalenceMap);
};

}  // namespace zucchini

#endif  // ZUCCHINI_EQUIVALENCE_MAP_H_
