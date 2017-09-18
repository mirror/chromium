// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/models/list_selection_model.h"

#include <algorithm>
#include <valarray>

#include "base/logging.h"
#include "base/stl_util.h"

namespace ui {

// static
const int ListSelectionModel::kUnselectedIndex = -1;

namespace {

void IncrementFromImpl(int index, int* value) {
  if (*value >= index)
    (*value)++;
}

// Returns true if |value| should be erased from its container.
bool DecrementFromImpl(int index, int* value) {
  if (*value == index) {
    *value = ListSelectionModel::kUnselectedIndex;
    return true;
  }
  if (*value > index)
    (*value)--;
  return false;
}

void MoveToLowerIndexImpl(int old_start,
                          int new_start,
                          int length,
                          int* value) {
  DCHECK_LE(new_start, old_start);
  DCHECK_GE(length, 0);
  // When a range of items moves to a lower index, the only affected indices
  // are those in the interval [new_start, old_start + length).
  if (new_start <= *value && *value < old_start + length) {
    if (*value < old_start) {
      // The items originally in the interval [new_start, old_start) see
      // |length| many items inserted before them, so their indices increase.
      *value += length;
    } else {
      // The items originally in the interval [old_start, old_start + length)
      // are shifted downward by (old_start - new_start) many spots, so
      // their indices decrease.
      *value -= (old_start - new_start);
    }
  }
}

}  // namespace

ListSelectionModel::ListSelectionModel()
    : active_(kUnselectedIndex),
      anchor_(kUnselectedIndex) {
}

ListSelectionModel::~ListSelectionModel() {
}

void ListSelectionModel::IncrementFrom(int index) {
  // Shift the selection to account for a newly inserted item at |index|.
  for (SelectedIndices::iterator i = selected_indices_.begin();
       i != selected_indices_.end(); ++i) {
    IncrementFromImpl(index, &(*i));
  }
  IncrementFromImpl(index, &anchor_);
  IncrementFromImpl(index, &active_);
}

void ListSelectionModel::DecrementFrom(int index) {
  for (SelectedIndices::iterator i = selected_indices_.begin();
       i != selected_indices_.end(); ) {
    if (DecrementFromImpl(index, &(*i)))
      i = selected_indices_.erase(i);
    else
      ++i;
  }
  DecrementFromImpl(index, &anchor_);
  DecrementFromImpl(index, &active_);
}

void ListSelectionModel::SetSelectedIndex(int index) {
  anchor_ = active_ = index;
  selected_indices_.clear();
  if (index != kUnselectedIndex)
    selected_indices_.push_back(index);
}

bool ListSelectionModel::IsSelected(int index) const {
  return base::ContainsValue(selected_indices_, index);
}

void ListSelectionModel::AddIndexToSelection(int index) {
  if (!IsSelected(index)) {
    selected_indices_.push_back(index);
    std::sort(selected_indices_.begin(), selected_indices_.end());
  }
}

void ListSelectionModel::RemoveIndexFromSelection(int index) {
  SelectedIndices::iterator i = std::find(selected_indices_.begin(),
                                          selected_indices_.end(), index);
  if (i != selected_indices_.end())
    selected_indices_.erase(i);
}

void ListSelectionModel::SetSelectionFromAnchorTo(int index) {
  if (anchor_ == kUnselectedIndex) {
    SetSelectedIndex(index);
  } else {
    int delta = std::abs(index - anchor_);
    SelectedIndices new_selection(delta + 1, 0);
    for (int i = 0, min = std::min(index, anchor_); i <= delta; ++i)
      new_selection[i] = i + min;
    selected_indices_.swap(new_selection);
    active_ = index;
  }
}

void ListSelectionModel::AddSelectionFromAnchorTo(int index) {
  if (anchor_ == kUnselectedIndex) {
    SetSelectedIndex(index);
  } else {
    for (int i = std::min(index, anchor_), end = std::max(index, anchor_);
         i <= end; ++i) {
      if (!IsSelected(i))
        selected_indices_.push_back(i);
    }
    std::sort(selected_indices_.begin(), selected_indices_.end());
    active_ = index;
  }
}

void ListSelectionModel::Move(int from, int to, int length) {
  // |length| many items are moving from index |from| to index |to|.
  DCHECK_NE(from, to);
  DCHECK_GT(length, 0);

  // Remap move-to-higher-index operations to the equivalent move-to-lower-index
  // operation. As an example, the permutation "ABCDEFG" -> "CDEFABG" can be
  // thought of either as shifting 'AB' higher by 4, or by shifting 'CDEF' lower
  // by 2.
  if (to > from) {
    Move(from + length, from, to - from);
    return;
  }

  // We know that from > to, so this is a move to a lower index. Start by
  // transforming |anchor_| and |active_|.
  MoveToLowerIndexImpl(from, to, length, &anchor_);
  MoveToLowerIndexImpl(from, to, length, &active_);

  // When a range of items moves to a lower index, the affected items are those
  // in the interval [to, from + length). Search within |selected_indices_| for
  // indices that fall into that range.
  auto low =
      std::lower_bound(selected_indices_.begin(), selected_indices_.end(), to);
  auto high = std::lower_bound(low, selected_indices_.end(), from + length);

  // The items originally in the interval [to, from) will see |length| many
  // items inserted before them, so their indices increase.
  auto middle = std::lower_bound(low, high, from);
  int pivot_value = to + length;
  for (auto it = low; it != middle; ++it) {
    (*it) += length;
    DCHECK(pivot_value <= (*it) && (*it) < (from + length));
  }

  // The items originally in the interval [from, from + length) are shifted
  // downward by (from - to) many spots, so their indices decrease.
  for (auto it = middle; it != high; ++it) {
    (*it) -= (from - to);
    DCHECK(to <= (*it) && (*it) < pivot_value);
  }

  // Reorder the ranges [low, middle), and [middle, high) so that the elements
  // in [middle, high) appear first, followed by [low, middle). This suffices to
  // restore the sort condition on |selected_indices_|, because each range is
  // still sorted piecewise, and |pivot_value| is a lower bound for elements in
  // [low, middle), and an upper bound for [middle, high).
  std::rotate(low, middle, high);
  DCHECK(std::is_sorted(selected_indices_.begin(), selected_indices_.end()));
}

void ListSelectionModel::Clear() {
  anchor_ = active_ = kUnselectedIndex;
  SelectedIndices empty_selection;
  selected_indices_.swap(empty_selection);
}

void ListSelectionModel::Copy(const ListSelectionModel& source) {
  selected_indices_ = source.selected_indices_;
  active_ = source.active_;
  anchor_ = source.anchor_;
}

bool ListSelectionModel::Equals(const ListSelectionModel& rhs) const {
  return active_ == rhs.active() &&
      anchor_ == rhs.anchor() &&
      selected_indices() == rhs.selected_indices();
}

}  // namespace ui
