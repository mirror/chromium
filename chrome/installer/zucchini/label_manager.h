// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_INSTALLER_ZUCCHINI_LABEL_MANAGER_H_
#define CHROME_INSTALLER_ZUCCHINI_LABEL_MANAGER_H_

#include <cstddef>
#include <unordered_map>
#include <vector>

#include "chrome/installer/zucchini/disassembler.h"
#include "chrome/installer/zucchini/image_utils.h"

namespace zucchini {

// A Label is an {offset, index} pair. A Label Manager stores a list of Labels
// with distinct offsets and distinct indexs, and provides functions to:
// - Get the offset of a stored index.
// - Get the index of a stored offset.
// - Insert Labels.
// - (Unsupported: Remove Labels, modify Labels).
// Offsets and indexes as encoded as integers. To enable mixing offsets and
// indexes while maintaining semantics, indexes are encoded ("marked") using
// MarkIndex(), and decoded using UnmarkIndex(). Therefore IsMarked() is used to
// distinguish indexes from offsets.

// Constant as placeholder for non-existing offset for an index.
constexpr offset_t kUnusedIndex = offset_t(-1);

// Base class for OrderedLabelManager and UnorderedLabelManager. We're not
// making common functions virtual, since polymorphism is unused and so we may
// as well avoid incurring the performance hit.
class BaseLabelManager {
 public:
  BaseLabelManager();
  virtual ~BaseLabelManager();
  explicit BaseLabelManager(const BaseLabelManager&);

  // Returns whether |offset_or_index| is a valid offset.
  static bool IsOffset(offset_t offset_or_index) {
    return offset_or_index != kUnusedIndex && !IsMarked(offset_or_index);
  }

  // Returns whether |offset_or_index| is a valid marked index.
  static bool IsMarkedIndex(offset_t offset_or_index) {
    return offset_or_index != kUnusedIndex && IsMarked(offset_or_index);
  }

  // Returns whether a given (unmarked) |index| is stored.
  bool IsIndexStored(offset_t index) const {
    return index < labels_.size() && labels_[index] != kUnusedIndex;
  }

  // Returns the offset of a given (unmarked) |index| if it is stored, or
  // |kUnusedIndex| otherwise.
  offset_t OffsetOfIndex(offset_t index) const {
    return index < labels_.size() ? labels_[index] : kUnusedIndex;
  }

  // If |offset_or_index| is a stored marked index, then replaces it by the
  // corresponding (unmarked) offset.
  offset_t OffsetFromMarkedIndex(offset_t offset_or_index) const;

  size_t size() const { return labels_.size(); }

 protected:
  // Main storage of distinct offsets, providing O(1) offset-of-index lookup.
  // UnorderedLabelManager may contain "gaps" with |kUnusedIndex|.
  std::vector<offset_t> labels_;
};

// OrderedLabelManager is a Label Manager that prioritizes memory efficiency,
// storing Labels as a sorted list of offsets in |labels_|. Label insertions
// are performed in batch to reduce costs. Index-of-offset lookup is O(lg n)
// (binary search).
class OrderedLabelManager : public BaseLabelManager {
 public:
  OrderedLabelManager();
  ~OrderedLabelManager() override;
  OrderedLabelManager(const OrderedLabelManager&);

  // If |offset| is stored, returns its index. Otherwise returns |kUnusedIndex|.
  offset_t IndexOfOffset(offset_t offset) const;

  // If |*index_or_offset| is a stored offset, then replaces it with the
  // corresponding marked index.
  offset_t MarkedIndexFromOffset(offset_t index_or_offset) const;

  // Adds each (unmarked) offset in |offsets| to |labels_|. This invalidates all
  // previous Label lookups.
  void InsertOffsets(const std::vector<offset_t>& offsets);

  // For each Reference in |refs|, insert its target offset to |labels_|. This
  // invalidates all previous Label lookups.
  void InsertTargets(const ReferenceGroup& refs, Disassembler* disasm);

  const std::vector<offset_t>& Labels() const { return labels_; }
};

// UnorderedLabelManager is a Label Manager that prioritizes speed for lookup
// and insertion, but uses more memory than OrderedLabelManager. In addition to
// using |labels_| to store *unsorted* distinct offsets, an unordered_map
// |labels_map_| is used for index-of-offset lookup.
class UnorderedLabelManager : public BaseLabelManager {
 public:
  UnorderedLabelManager();
  ~UnorderedLabelManager() override;
  UnorderedLabelManager(const UnorderedLabelManager&);

  // If |offset| is stored, returns its index. Otherwise returns |kUnusedIndex|.
  offset_t IndexOfOffset(offset_t offset) const;

  // If |index_or_offset| is a stored offset, then replaces it with the
  // corresponding marked index.
  offset_t MarkedIndexFromOffset(offset_t index_or_offset) const;

  // Assumes |labels| consists of unique offsets, but may have "gaps" in the
  // form of |kUnusedIndex|. Clears and reinitializes all stored data.
  void Init(std::vector<offset_t>&& labels);

  // Assumes |offset| is not a stored (unmarked) offset, and adds |offset| to
  // |labels_|. If |kUnusedIndex| gaps exist, try to fill them with |offset|;
  // otherwise appends |offset| to end of |labels_|. Previous lookup results
  // involving stored offsets / indexes remain valid.
  void InsertNewOffset(offset_t offset);

 private:
  // Inverse map of |labels_| (excludes |kUnusedIndex|).
  std::unordered_map<offset_t, offset_t> labels_map_;

  // Index into |label_| to scan for |kUnusedIndex| entry in |labels_|.
  size_t gap_idx_ = 0;
};

}  // namespace zucchini

#endif  // CHROME_INSTALLER_ZUCCHINI_LABEL_MANAGER_H_
