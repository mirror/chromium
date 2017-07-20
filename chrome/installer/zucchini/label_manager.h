// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_INSTALLER_ZUCCHINI_LABEL_MANAGER_H_
#define CHROME_INSTALLER_ZUCCHINI_LABEL_MANAGER_H_

#include <stddef.h>

#include <unordered_map>
#include <vector>

#include "chrome/installer/zucchini/image_utils.h"

namespace zucchini {

class ReferenceGroup;
class Disassembler;

// Zucchini matches reference targets (as offsets) between "old" and "new"
// images. To represent these, we:
// (1) Define a Label as an {offset, index} pair, meaning that |index| is
//     assigned to |offset|.
// (2) Assign an index to each target offset in "old".
// (3) Do the same for "new".
// (4) Represent matching "old" offsets and "new" offsets as "old" Labels and
//     "new" Labels that share a common index.
// For example, suppose "old" Labels are:
//   {0x1111, 0}, {0x3333, 4}, {0x5555, 1}, {0x7777, 3}
// and "new" Labels are:
//   {0x2222, 4}, {0x4444, 8}, {0x6666, 0}, {0x8888, 2}
// Then the matched offsets between "old" and "new" are:
//   0x1111 <=> 0x6666,  0x333 <=> 0x4444.

// A LabelManager stores a list of Labels with distinct offsets and distinct
// indices for an image ("old" or "new"). It also provides functions to:
// - Get the offset of a stored index.
// - Get the index of a stored offset.
// - Insert Labels.
// - (Unsupported: Remove Labels, modify Labels).
// A LabelManager allows offsets to be represented as indexes. To enable
// mixed representation of targets as offsets or indexes, indexes are stored
// as "marked" values (implemented by setting the MSB), and offsets are
// "unmarked". So when working with stored targets:
// - IsMarked() distinguishes offsets (false) from indexes (true).
// - MarkIndex() is used to encode indexes to their stored value.
// - UnmarkIndex() is used to decode indexes to their actual value.
// - Target offsets are stored verbatim, but they must not be marked. This
//   affects reference parsing, where we reject all references whose offsets
//   happen to be marked.

// Constant as placeholder for non-existing offset for an index.
constexpr offset_t kUnusedIndex = offset_t(-1);

// Base class for OrderedLabelManager and UnorderedLabelManager. We're not
// making common functions virtual, since polymorphism is unused and so we may
// as well avoid incurring the performance hit.
class BaseLabelManager {
 public:
  BaseLabelManager();
  BaseLabelManager(const BaseLabelManager&);
  virtual ~BaseLabelManager();

  // Returns whether |offset_or_marked_index| is a valid offset.
  static constexpr bool IsOffset(offset_t offset_or_marked_index) {
    return offset_or_marked_index != kUnusedIndex &&
           !IsMarked(offset_or_marked_index);
  }

  // Returns whether |offset_or_marked_index| is a valid marked index.
  static constexpr bool IsMarkedIndex(offset_t offset_or_marked_index) {
    return offset_or_marked_index != kUnusedIndex &&
           IsMarked(offset_or_marked_index);
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

  // If |offset_or_marked_index| is a stored marked index, then returns the
  // corresponding (unmarked) offset. Otherwise, returns
  // |offset_or_marked_index|.
  offset_t OffsetFromMarkedIndex(offset_t offset_or_marked_index) const;

  size_t size() const { return labels_.size(); }

 protected:
  // Main storage of distinct offsets, providing O(1) offset-of-index lookup.
  // UnorderedLabelManager may contain "gaps" with |kUnusedIndex|.
  std::vector<offset_t> labels_;
};

// OrderedLabelManager is a LabelManager that prioritizes memory efficiency,
// storing Labels as a sorted list of offsets in |labels_|. Label insertions
// are performed in batch to reduce costs. Index-of-offset lookup is O(lg n)
// (binary search).
class OrderedLabelManager : public BaseLabelManager {
 public:
  OrderedLabelManager();
  OrderedLabelManager(const OrderedLabelManager&);
  ~OrderedLabelManager() override;

  // If |offset| is stored, returns its index. Otherwise returns |kUnusedIndex|.
  offset_t IndexOfOffset(offset_t offset) const;

  // If |offset_or_marked_index| is a stored offset, then returns the
  // corresponding marked index. Otherwise, returns |offset_or_marked_index|.
  offset_t MarkedIndexFromOffset(offset_t offset_or_marked_index) const;

  // Adds each offset in |offsets| to |labels_|. This invalidates all previous
  // Label lookups.
  void InsertOffsets(const std::vector<offset_t>& offsets);

  // For each Reference in |refs|, insert its target offset to |labels_|. This
  // invalidates all previous Label lookups.
  void InsertTargets(const ReferenceGroup& refs, Disassembler* disasm);

  const std::vector<offset_t>& Labels() const { return labels_; }
};

// UnorderedLabelManager is a LabelManager that prioritizes speed for lookup
// and insertion, but uses more memory than OrderedLabelManager. In addition to
// using |labels_| to store *unsorted* distinct offsets, an unordered_map
// |labels_map_| is used for index-of-offset lookup.
class UnorderedLabelManager : public BaseLabelManager {
 public:
  UnorderedLabelManager();
  UnorderedLabelManager(const UnorderedLabelManager&);
  ~UnorderedLabelManager() override;

  // If |offset| is stored, returns its index. Otherwise returns |kUnusedIndex|.
  offset_t IndexOfOffset(offset_t offset) const;

  // If |offset_or_marked_index| is a stored offset, then returns the
  // corresponding marked index. Otherwise, returns |offset_or_marked_index|.
  offset_t MarkedIndexFromOffset(offset_t offset_or_marked_index) const;

  // Clears and reinitializes all stored data. //Assumes |labels| consists of
  // unique offsets, but may have "gaps" in the form of |kUnusedIndex|.
  void Init(std::vector<offset_t>&& labels);

  // Assumes |offset| is not a stored (unmarked) offset, and adds |offset| to
  // |labels_|. If |kUnusedIndex| gaps exist, tries to fill them with |offset|;
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
