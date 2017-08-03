// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_INSTALLER_ZUCCHINI_ENCODED_VIEW_H_
#define CHROME_INSTALLER_ZUCCHINI_ENCODED_VIEW_H_

#include <stddef.h>
#include <stdint.h>

#include <iterator>

#include "chrome/installer/zucchini/image_index.h"
#include "chrome/installer/zucchini/image_utils.h"

namespace zucchini {

constexpr size_t kReferencePaddingRank = 256;
constexpr size_t kBaseRefRank = 257;

// Utility to adapt an image file so references get aliased with their label
// indices. This is useful to find matched blocks where pointers are forgiven
// easily between two image files.
class EncodedView {
 public:
  class Iterator {
   public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = size_t;
    using difference_type = ptrdiff_t;
    using reference = size_t;
    using pointer = size_t*;

    Iterator(const ImageIndex* image_index, difference_type pos)
        : image_index_(image_index), pos_(pos) {}

    value_type operator*() const {
      return EncodedView::Rank(*image_index_, static_cast<offset_t>(pos_));
    }

    value_type operator[](difference_type n) const {
      return EncodedView::Rank(*image_index_, static_cast<offset_t>(pos_ + n));
    }

    Iterator& operator++() {
      ++pos_;
      return *this;
    }

    Iterator operator++(int) {
      Iterator tmp = *this;
      ++pos_;
      return tmp;
    }

    Iterator& operator--() {
      --pos_;
      return *this;
    }

    Iterator operator--(int) {
      Iterator tmp = *this;
      --pos_;
      return tmp;
    }

    Iterator& operator+=(difference_type n) {
      pos_ += n;
      return *this;
    }

    Iterator& operator-=(difference_type n) {
      pos_ -= n;
      return *this;
    }

    friend bool operator==(Iterator a, Iterator b) { return a.pos_ == b.pos_; }

    friend bool operator!=(Iterator a, Iterator b) { return !(a == b); }

    friend bool operator<(Iterator a, Iterator b) { return a.pos_ < b.pos_; }

    friend bool operator>(Iterator a, Iterator b) { return b < a; }

    friend bool operator<=(Iterator a, Iterator b) { return !(b < a); }

    friend bool operator>=(Iterator a, Iterator b) { return !(a < b); }

    friend difference_type operator-(Iterator a, Iterator b) {
      return a.pos_ - b.pos_;
    }

    friend Iterator operator+(Iterator it, difference_type n) {
      it += n;
      return it;
    }

    friend Iterator operator-(Iterator it, difference_type n) {
      it -= n;
      return it;
    }

   private:
    const ImageIndex* image_index_;
    difference_type pos_;
  };

  using value_type = size_t;
  using size_type = offset_t;
  using difference_type = ptrdiff_t;
  using const_iterator = Iterator;

  // Projects a location (offset in file) to a rank associated with the data at
  // this index. If index points to raw data, its rank is the value at this
  // index. Otherwise, index points to a part of reference and its rank will
  // encode its type and its target.
  static value_type Rank(const ImageIndex& image_index, offset_t location);

  // Instances of this class are bound to an image file and its associated
  // container of References owned by the caller.
  explicit EncodedView(const ImageIndex* image_index)
      : image_index_(image_index) {}

  // Returns the cardinality of the projection, that is the maximum possible
  // rank + 1.
  value_type Cardinality() const;

  size_type size() const { return size_type(image_index_->size()); }
  const_iterator begin() const {
    return const_iterator{image_index_, difference_type(0)};
  }
  const_iterator end() const {
    return const_iterator{image_index_, difference_type(size())};
  }

 private:
  const ImageIndex* image_index_;
};

}  // namespace zucchini

#endif  // CHROME_INSTALLER_ZUCCHINI_ENCODED_VIEW_H_
