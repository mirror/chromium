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

// Transforms an index according to the semantics of std::rotate(new_start,
// old_start, end). This assumes that old_start < new_start < end.
static void RotateIndices(int new_start, int old_start, int end, int* value) {
  DCHECK(new_start < old_start);
  if (*value >= new_start && *value < end && new_start != old_start &&
      old_start != end) {
    if (*value < old_start)
      *value += (end - old_start);
    else
      *value -= (old_start - new_start);
  }
}

static bool DecrementFromImpl(int index, int* value) {
  if (*value == index) {
    *value = ListSelectionModel::kUnselectedIndex;
    return true;
  }
  if (*value > index)
    (*value)--;
  return false;
}

ListSelectionModel::ListSelectionModel()
    : active_(kUnselectedIndex),
      anchor_(kUnselectedIndex) {
}

ListSelectionModel::~ListSelectionModel() {
}

void ListSelectionModel::IncrementFrom(int index) {
  Move(std::numeric_limits<int>::max() - 1, index, 1);
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
  DCHECK(length > 0);
  if (to == from)
    return;

  // Remap move-to-right operations to the equivalent move-left operation. As an
  // example, the permutation "ABCDEFG" -> "CDEFABG" can be thought of either as
  // shifting 'AB' right by 4, or by shifting 'CDEF' left by 2.
  if (to > from) {
    Move(from + length, from, to - from);
    return;
  }

  auto low =
      std::lower_bound(selected_indices_.begin(), selected_indices_.end(), to);
  auto high = std::lower_bound(low, selected_indices_.end(), from + length);
  auto middle = std::lower_bound(low, high, from);

  for (auto it = low; it != high; ++it)
    RotateIndices(to, from, from + length, &(*it));

  // Reorder the ranges [low, middle), and [middle, high). Each range is sorted
  // piecewise, and every elements in [low, middle) is less than every element
  // in [middle, high), so swapping the ranges restores the sort order.
  std::rotate(low, middle, high);
  DCHECK(std::is_sorted(selected_indices_.begin(), selected_indices_.end()));

  // Apply this same transformation to |anchor_| and |active_|.
  RotateIndices(to, from, from + length, &anchor_);
  RotateIndices(to, from, from + length, &active_);
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
