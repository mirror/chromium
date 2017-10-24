// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/tabs/tab_strip_grouper.h"

#include <algorithm>

#include "chrome/browser/ui/tabs/tab_strip_grouper_observer.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"

namespace {

// Implements < for the model indices in two gruper items.
struct ItemModelIndexLess {
  bool operator()(const TabStripGrouperItem& left, int right_index) const {
    return left.model_index() < right_index;
  }
};

}  // namespace

TabStripGrouper::TabStripGrouper(TabStripModel* model) : model_(model) {
  model_->AddObserver(this);
  Regroup();
}

TabStripGrouper::~TabStripGrouper() {
  model_->RemoveObserver(this);
}

bool TabStripGrouper::ContainsIndex(int item_index) const {
  return item_index >= 0 && item_index < count();
}

TabStripGrouperItem* TabStripGrouper::GetItemAt(int index) {
  DCHECK(index >= 0 && index < static_cast<int>(items_.size()));
  return &items_[index];
}

int TabStripGrouper::ItemIndexForModelIndex(int model_index) const {
  if (model_index == -1)
    return -1;  // -1 is used for some "not present" values.
  if (items_.empty())
    return 0;
  auto lower = std::lower_bound(items_.begin(), items_.end(), model_index,
                                ItemModelIndexLess());

  // lower_bound will have returned either a matching index or the next bigger
  // one. In the latter case, need to find the previous group.
  if (lower == items_.end() || lower->model_index() > model_index) {
    // Since the first item should always have a model index of 0, it will never
    // satisfy the condition of being larger than the input index so we can
    // always safely subtract 1 from the iterator.
    auto previous = lower - 1;
    if (previous->type() == TabStripGrouperItem::Type::GROUP &&
        model_index < previous->model_index() +
                          static_cast<int>(previous->items().size()))
      --lower;
  }
  return lower - items_.begin();
}

int TabStripGrouper::ModelIndexForItemIndex(int item_index) const {
  if (item_index == -1)
    return -1;  // -1 is used for some "not present" values.
  if (item_index >= count())
    return model_->count();
  return items_[item_index].model_index();
}

void TabStripGrouper::SetViewParams(const ViewParams& params) {
  view_params_ = params;
}

void TabStripGrouper::AddObserver(TabStripGrouperObserver* observer) {
  observers_.AddObserver(observer);
}

void TabStripGrouper::RemoveObserver(TabStripGrouperObserver* observer) {
  observers_.RemoveObserver(observer);
}

bool TabStripGrouper::Regroup() {
  std::vector<TabStripGrouperItem> new_items = ComputeGroups();
  if (new_items == items_)
    return false;

  std::vector<int> removed;
  std::vector<int> added;

  size_t old_item_index = 0;
  size_t new_item_index = 0;
  while (old_item_index < items_.size() || new_item_index < new_items.size()) {
    if (old_item_index < items_.size() &&        // Has an old item &&
        (new_item_index == new_items.size() ||   // No new item ||
         items_[old_item_index].model_index() <  // New index skips ahead.
             new_items[new_item_index].model_index())) {
      // Old array has an index not present in the new one. This item was
      // removed.
      removed.push_back(old_item_index);
      old_item_index++;
    } else if (new_item_index < new_items.size() &&        // Has a new item &&
               (old_item_index == items_.size() ||         // No old item ||
                new_items[new_item_index].model_index() <  // Old item skips.
                    items_[old_item_index].model_index())) {
      // New array has an index not present, this one was added.
      added.push_back(new_item_index);
      new_item_index++;
    } else {
      // The model indices must match.
      DCHECK_EQ(new_items[new_item_index].model_index(),
                items_[old_item_index].model_index());
      if (new_items[new_item_index] != items_[old_item_index]) {
        // Changed item, issue a remove and add.
        removed.push_back(old_item_index);
        added.push_back(new_item_index);
      }

      old_item_index++;
      new_item_index++;
    }
  }

  items_ = std::move(new_items);
  for (auto& observer : observers_)
    observer.ItemsChanged(removed, added);

  return true;
}

// This function computes the ideal set of groups given the current model. It
// does not replace the current groups or depend on it.
std::vector<TabStripGrouperItem> TabStripGrouper::ComputeGroups() const {
  std::vector<TabStripGrouperItem> items;
  items.reserve(model_->count());

  const ui::ListSelectionModel& model_selection = model_->selection_model();

  int model_index = 0;
  while (model_index < model_->count()) {
    // Eat all tabs with the same group.
    int64_t current_group_id = model_->GetPersistentGroupIdAt(model_index);
    int group_last_index = model_index;
    while (group_last_index < model_->count() - 1 &&
           model_->GetPersistentGroupIdAt(group_last_index + 1) ==
               current_group_id) {
      group_last_index++;
    }

    if (group_last_index - model_index >= 1) {
      // A group exists when there is more than one thing. But if any item in
      // the group is selected, ungroup all items.
      bool group_is_selected = false;
      for (int i = model_index; i <= group_last_index; i++)
        group_is_selected |= model_selection.IsSelected(i);
      if (group_is_selected) {
        // Selected group, add all members as individual tabs.
        for (int i = model_index; i <= group_last_index; i++)
          items.emplace_back(model_->GetWebContentsAt(i), i);
      } else {
        // Unselected, make the group.
        TabStripGrouperItem group;
        group.type_ = TabStripGrouperItem::Type::GROUP;
        group.model_index_ = model_index;

        group.items_.reserve(group_last_index - model_index + 1);
        for (int i = model_index; i <= group_last_index; i++)
          group.items_.emplace_back(model_->GetWebContentsAt(i), i);

        items.push_back(std::move(group));
      }

      // Pick up with the item following the patching opener.
      model_index = group_last_index + 1;
      continue;
    }

    items.emplace_back(model_->GetWebContentsAt(model_index), model_index);
    model_index++;
  }
  return items;
}

ui::ListSelectionModel TabStripGrouper::ItemSelectionForModelSelection(
    const ui::ListSelectionModel& source) const {
  ui::ListSelectionModel dest;

  dest.set_anchor(ItemIndexForModelIndex(source.anchor()));
  dest.set_active(ItemIndexForModelIndex(source.active()));
  for (int index : source.selected_indices())
    dest.AddIndexToSelection(ItemIndexForModelIndex(index));

  return dest;
}

void TabStripGrouper::ShiftModelIndices(int starting_at, int delta) {
  for (int i = ItemIndexForModelIndex(starting_at);
       i < static_cast<int>(items_.size()); i++)
    items_[i].model_index_ += delta;
}

void TabStripGrouper::TabInsertedAt(TabStripModel* tab_strip_model,
                                    content::WebContents* contents,
                                    int model_index,
                                    bool foreground) {
  // Don't need to issue a notification. Leave a hole in the items_ list and
  // Regroup() will notice if something got added. If the item is added inside
  // an existing group, no notification should be issued.
  ShiftModelIndices(model_index, 1);
  Regroup();
}

void TabStripGrouper::TabClosingAt(TabStripModel* tab_strip_model,
                                   content::WebContents* contents,
                                   int model_index) {
  // Mark that item with a -1 index and shift all other model indices to
  // account for the change. Regroup() will notice the -1 and issue a deleted
  // notification for it.
  int item_index = ItemIndexForModelIndex(model_index);
  if (items_[item_index].type() == TabStripGrouperItem::Type::TAB) {
    DCHECK(items_[item_index].web_contents() == contents);
    items_[item_index].model_index_ = -1;
  }
  ShiftModelIndices(model_index, -1);
  Regroup();
}

void TabStripGrouper::TabDetachedAt(content::WebContents* contents,
                                    int model_index) {
  TabClosingAt(model_, contents, model_index);
}

void TabStripGrouper::TabDeactivated(content::WebContents* contents) {}

void TabStripGrouper::ActiveTabChanged(content::WebContents* old_contents,
                                       content::WebContents* new_contents,
                                       int model_index,
                                       int reason) {}

void TabStripGrouper::TabSelectionChanged(
    TabStripModel* tab_strip_model,
    const ui::ListSelectionModel& old_model) {
  // The new model has to be up-to-date before nofifying observers.
  selection_model_ = ItemSelectionForModelSelection(model_->selection_model());

  // If changing the selection changed groups, assume we don't need to issue
  // a separate selection notification. If the groups changed, the "old_model"
  // will be out-of-date anyway.
  if (Regroup())
    return;

  ui::ListSelectionModel old_item_model =
      ItemSelectionForModelSelection(old_model);
  for (auto& observer : observers_)
    observer.ItemSelectionChanged(old_item_model);
}

void TabStripGrouper::TabMoved(content::WebContents* contents,
                               int from_model_index,
                               int to_model_index) {
  Regroup();
}

void TabStripGrouper::TabChangedAt(content::WebContents* contents,
                                   int model_index,
                                   TabChangeType change_type) {
  int item_index = ItemIndexForModelIndex(model_index);
  for (auto& observer : observers_)
    observer.ItemUpdatedAt(item_index, change_type);
}

void TabStripGrouper::TabReplacedAt(TabStripModel* tab_strip_model,
                                    content::WebContents* old_contents,
                                    content::WebContents* new_contents,
                                    int model_index) {
  Regroup();
}

void TabStripGrouper::TabPinnedStateChanged(TabStripModel* tab_strip_model,
                                            content::WebContents* contents,
                                            int model_index) {
  TabChangedAt(contents, model_index, TabStripModelObserver::ALL);
}

void TabStripGrouper::TabBlockedStateChanged(content::WebContents* contents,
                                             int model_index) {
  TabChangedAt(contents, model_index, TabStripModelObserver::ALL);
}

void TabStripGrouper::TabStripEmpty() {}

void TabStripGrouper::WillCloseAllTabs() {}

void TabStripGrouper::CloseAllTabsCanceled() {}

void TabStripGrouper::TabNeedsAttentionAt(int model_index) {
  int item_index = ItemIndexForModelIndex(model_index);
  for (auto& observer : observers_)
    observer.ItemNeedsAttentionAt(item_index);
}
