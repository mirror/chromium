// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ZUCCHINI_LABEL_MANAGER_H_
#define ZUCCHINI_LABEL_MANAGER_H_

#include <algorithm>
#include <cassert>
#include <unordered_map>
#include <utility>
#include <vector>

#include "zucchini/disassembler.h"
#include "zucchini/image_utils.h"
#include "zucchini/ranges/algorithm.h"
#include "zucchini/ranges/functional.h"
#include "zucchini/ranges/view.h"

namespace zucchini {

// LabelManagers provide utilities to replace values by indices into a value
// table in which duplicates are removed, which we call label.
// They come in 2 flavors : Ordered and Unordered.

// Some indices in the label table might be unused, that is there is no value
// at that index. In this case, the label table will hold |kUnusedIndex|.
constexpr offset_t kUnusedIndex = offset_t(-1);

// Provides common functionnality for both OrderedLabelManager
// and UnorderedLabelManager. This class is not meant to use with polymorphism.
// It might make more sense to use composition or private inheritance?
class BaseLabelManager {
 public:
  BaseLabelManager();
  virtual ~BaseLabelManager();

  // Given a ForwardRange |refs|, replaces marked indices found in |refs| by
  // their original values.
  template <class Rng>
  void Unassign(Rng&& refs) const;

  // Replaces |ref| with its original value if it is a marked index,
  void Unassign(offset_t* ref) const {
    if (!IsMarked(*ref))
      return;
    *ref = labels_[UnmarkIndex(*ref)];
  }

  void clear() { labels_.clear(); }
  offset_t at(offset_t idx) const { return labels_[idx]; }
  offset_t operator[](offset_t idx) const { return labels_[idx]; }
  size_t size() const { return labels_.size(); }

  // Access label table.
  const std::vector<offset_t>& Labels() const { return labels_; }

 protected:
  std::vector<offset_t> labels_;
};

// OrderedLabelManager maintains an ordered address table and uses binary search
// to assign labels. It is less memory hungry, but is more restrictive
// to ensure an ordered table.
class OrderedLabelManager : public BaseLabelManager {
 public:
  // Given a ForwardRange |refs|, replaces unmarked values found in |refs| by
  // their indices if those values are present in the label table.
  template <class Rng>
  void Assign(Rng&& refs) const;
  // Replaces |ref| with its index if it is an unmarked value present in the
  // label table.
  void Assign(offset_t* ref) const;

  // Given a ForwardRange |refs|, add all unique unmarked values found in |refs|
  // to the label table. Existing labels might be moved, thus invalidating
  // all previous assignments.
  template <class Rng>
  void Allocate(Rng&& refs);
  // Given a ReferenceGenerator, add all unique values obtained by projecting
  // References using |proj|'s call operator to the label table. Existing labels
  // might be moved, thus invalidating all previous assignments.
  template <class Proj>
  void Allocate(const ReferenceGroup& refs, Proj proj);

  // Given a ForwardRange |refs|, add all unique unmarked values found in |refs|
  // to the label table and replaces them with their indices. Existing labels
  // might be moved, thus invalidating all previous assignments.
  template <class Rng>
  void AllocateAndAssign(Rng&& refs);
};

// UnorderedLabelManager uses an additionnal unordered_map to assign labels.
// When a value gets replaced by its index, this value gets Marked by a flag.
class UnorderedLabelManager : public BaseLabelManager {
 public:
  UnorderedLabelManager();
  ~UnorderedLabelManager() override;

  // Given a ForwardRange |refs|, replaces unmarked values found in |refs| by
  // their indices if those values are present in the label table.
  template <class Rng>
  void Assign(Rng&& refs);
  // Replaces |ref| with its index if it is an unmarked value present in the
  // label table.
  void Assign(offset_t* ref);

  // Given a ForwardRange |refs|, add all unique unmarked values found in |refs|
  // to the label table. Calls |emit|, passing each new label as argument.
  template <class Rng>
  void Allocate(Rng&& refs);

  // Given a ForwardRange |refs|, add all unique unmarked values found in |refs|
  // to the label table if not already present, replaces them with their indices
  // otherwise. Calls |emit|, passing each new label as argument.
  template <class Rng>
  void AssignOrAllocate(Rng&& refs);

  // Given a ForwardRange |refs|, add all unique unmarked values found in |refs|
  // to the label table and replaces them with their indices. Calls |emit|,
  // passing each new label as argument.
  template <class Rng>
  void AllocateAndAssign(Rng&& refs);

  // Inits the label table with |labels|.
  void Init(std::vector<offset_t>&& labels) {
    clear();
    labels_ = std::move(labels);
  }

  // Given a ForwardRange |refs| of unique unmark values, adds them to the label
  // table in the same order they are given. Labels already present in
  // the label table values are not replaced, but skipped.
  template <class Rng>
  void Digest(Rng&& refs);

  void clear();

 private:
  void InPlaceDigest();
  void UpdateMap();

  // Inverse map of |labels_|, but ignoring |kUnusedIndex|.
  std::unordered_map<offset_t, offset_t> labels_map_;

  // Index of first value in |labels_| whose value is not in |labels_map_|.
  size_t first_unindexed_label_ = 0;

  // Index of first |kUnusedIndex| in |labels_|.
  size_t first_unused_idx_ = 0;
};

template <class Rng>
void BaseLabelManager::Unassign(Rng&& refs) const {
  for (auto&& ref : refs) {
    Unassign(&ref);
  }
}

template <class Rng>
void OrderedLabelManager::Assign(Rng&& refs) const {
  if (labels_.empty())
    return;
  for (auto&& ref : refs) {
    if (IsMarked(ref))
      continue;
    auto it = ranges::lower_bound(labels_, ref);
    if (it != labels_.end() && *it == ref) {
      ref = MarkIndex(it - labels_.begin());
    }
  }
}

template <class Rng>
void OrderedLabelManager::Allocate(Rng&& refs) {
  using ranges::begin;
  using ranges::end;

  ranges::copy(RemoveIf(refs, IsMarked), std::back_inserter(labels_));
  ranges::sort(labels_);
  labels_.erase(end(ranges::unique(labels_)), labels_.end());
  labels_.shrink_to_fit();
}

// TODO(huangs): Remove |proj|?
template <class Proj>
void OrderedLabelManager::Allocate(const ReferenceGroup& group, Proj proj) {
  // auto op = ranges::as_function(std::move(proj));
  auto refs = group.Find();
  for (Reference ref; refs(&ref);) {
    labels_.push_back(proj(ref));
  }
  ranges::sort(labels_);
  labels_.erase(std::end(ranges::unique(labels_)), labels_.end());
  labels_.shrink_to_fit();
}

template <class Rng>
void OrderedLabelManager::AllocateAndAssign(Rng&& refs) {
  Allocate(std::forward<Rng>(refs));
  Assign(std::forward<Rng>(refs));
}

template <class Rng>
void UnorderedLabelManager::Assign(Rng&& refs) {
  if (labels_.empty())
    return;
  UpdateMap();
  for (auto&& ref : refs) {
    if (IsMarked(ref))
      continue;
    auto it = labels_map_.find(ref);
    if (it != labels_map_.end()) {
      ref = MarkIndex(it->second);
    }
  }
}

template <class Rng>
void UnorderedLabelManager::Allocate(Rng&& refs) {
  using ranges::begin;
  using ranges::end;
  UpdateMap();
  for (auto ref : refs) {
    if (IsMarked(ref))
      continue;
    auto it = labels_map_.find(ref);
    if (it == labels_map_.end()) {
      labels_.push_back(ref);
    }
  }
  if (labels_.size() > first_unindexed_label_) {
    std::sort(labels_.begin() + first_unindexed_label_, labels_.end());
    labels_.erase(
        std::unique(labels_.begin() + first_unindexed_label_, labels_.end()),
        labels_.end());
    InPlaceDigest();
  }
}

template <class Rng>
void UnorderedLabelManager::AssignOrAllocate(Rng&& refs) {
  using ranges::begin;
  using ranges::end;
  UpdateMap();
  for (auto&& ref : refs) {
    if (IsMarked(ref))
      continue;
    auto it = labels_map_.find(ref);
    if (it != labels_map_.end()) {
      ref = MarkIndex(it->second);
    } else {
      labels_.push_back(ref);
    }
  }
  if (labels_.size() > first_unindexed_label_) {
    std::sort(labels_.begin() + first_unindexed_label_, labels_.end());
    labels_.erase(
        std::unique(labels_.begin() + first_unindexed_label_, labels_.end()),
        labels_.end());
    InPlaceDigest();
  }
}

template <class Rng>
void UnorderedLabelManager::AllocateAndAssign(Rng&& refs) {
  AssignOrAllocate(refs);
  Assign(refs);
}

template <class Rng>
void UnorderedLabelManager::Digest(Rng&& refs) {
  using ranges::begin;
  using ranges::end;

  first_unindexed_label_ = 0;
  auto it = begin(refs);
  while (it != end(refs)) {
    if (first_unused_idx_ >= labels_.size()) {
      labels_.push_back(*it);
      ++it;
    } else if (labels_[first_unused_idx_] == kUnusedIndex) {
      labels_[first_unused_idx_] = *it;
      ++it;
    }
    ++first_unused_idx_;
  }
  labels_.shrink_to_fit();
}

}  // namespace zucchini

#endif  // ZUCCHINI_LABEL_MANAGER_H_
