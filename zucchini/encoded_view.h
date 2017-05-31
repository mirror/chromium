// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ZUCCHINI_ENCODED_VIEW_H_
#define ZUCCHINI_ENCODED_VIEW_H_

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <vector>

#include "zucchini/image_utils.h"
#include "zucchini/label_manager.h"
#include "zucchini/ranges/iterator_facade.h"
#include "zucchini/reference_holder.h"
#include "zucchini/region.h"

namespace zucchini {

constexpr size_t kReferencePaddingRank = 256;
constexpr size_t kBaseRefRank = 257;

constexpr int kMismatchFatal = -1;
// Those values control that cost of forgiving mistakes.
// TODO(etiennep) Find the best values. Or perharp let the Disassembler specify
// them.
constexpr int kMismatchReference = 2;
constexpr int kMismatchRaw = 2;

// Utility to adapt an image file so references get aliased
// with their label indices.
// This is useful to find matched blocks where pointers are forgiven easily
// between two image files.
class EncodedView {
 public:
  class Cursor : public ranges::BaseCursor<Cursor> {
   public:
    using iterator_category = std::random_access_iterator_tag;

    Cursor(const EncodedView* view, std::ptrdiff_t idx)
        : view_(view), idx_(idx) {}
    size_t get() const { return view_->Rank(idx_); }
    size_t at(std::ptrdiff_t n) const { return view_->Rank(idx_ + n); }
    void advance(std::ptrdiff_t n) { idx_ += n; }
    std::ptrdiff_t distance(Cursor that) const { return idx_ - that.idx_; }

   private:
    const EncodedView* view_;
    std::ptrdiff_t idx_;
  };

  using const_iterator = ranges::IteratorFacade<Cursor>;
  using size_type = offset_t;

  // Instances of this class are bound to an image file and its associated
  // container of References owned by the caller.
  EncodedView(Region image,
              const ReferenceHolder& refs,
              size_t label_count = 1);
  ~EncodedView();

  // Computes the distance between |a| and |b|. This function is symmetric.
  // Returns 0 if |a| and |b| are equivalent. Returns kFatalMatch if |a| and |b|
  // cannot be matched. Returns a value greater than 0 otherwise. When trying to
  // match two image files, we wish to minimize accumulated distance of matched
  // blocks. The distance metric is computed so patches are well compressible.
  int Distance(size_t a, size_t b) const;

  // Projects an index (offset in file) to a rank associated with the data at
  // this index. If index points to raw data, its rank is the value at this
  // index. Otherwise, index points to a part of reference and its rank will
  // encode its type and its target.
  size_t Rank(offset_t idx) const;

  // Returns true if idx points to the start of a Token.
  bool IsToken(offset_t idx) const;

  // Returns the cardinality of the projection, that is the maximum possible
  // rank + 1.
  size_t Cardinality() const;

  void SetLabelCount(uint8_t pool, size_t label_count);

  offset_t size() const { return offset_t(image_.size()); }
  const_iterator begin() const { return const_iterator{this, 0}; }
  const_iterator end() const {
    return const_iterator{this, std::ptrdiff_t(size())};
  }
  const_iterator cbegin() const { return begin(); }
  const_iterator cend() const { return end(); }

 private:
  const ReferenceHolder* references_;
  Region image_;
  std::vector<uint8_t> types_;  // Used for random access lookup of ranks.
  std::vector<size_t> label_count_;
};

}  // namespace zucchini

#endif  // ZUCCHINI_ENCODED_VIEW_H_
