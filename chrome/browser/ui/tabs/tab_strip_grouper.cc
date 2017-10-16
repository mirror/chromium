// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/tabs/tab_strip_grouper.h"

#include <algorithm>

namespace {

// Implements < for the model indices in two gruper items.
struct ItemModelIndexLess {
  bool operator()(const TabStripGrouperItem& left, int right_index) const {
    return left.model_index() < right_index;
  }
};

}  // namespace

TabStripGrouper::TabStripGrouper(TabStripModel* model) : model_(model) {
  Regroup();
}

TabStripGrouper::~TabStripGrouper() {}

TabStripGrouperItem* TabStripGrouper::GetItemAt(int index) {
  DCHECK(index >= 0 && index < static_cast<int>(items_.size()));
  return &items_[index];
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

void TabStripGrouper::Regroup() {
  std::vector<TabStripGrouperItem> new_items = ComputeGroups();
  if (new_items == items_)
    return;  // Common case is nothing changed.

  // Insert a tab with no grouping:
  //   Old:  Tab @ 0  |  Tab @ 2  |  Group @ 2
  //   New:  Tab @ 0  |  Tab @ 1  |  Tab @ 2  |  Group @ 2
  //   -> Issue notification for insert @ 1 of 1 tabs.
  //
  // Remove a tab with no grouping:
  //   Old:  Tab @ 0  |  Tab @ 1  |  Tab @ 2  |  Group @ 2
  //   New:  Tab @ 0  |  Tab @ 2  |  Group @ 2
  //   -> Issue notification for removal of 
  //
  // Collapse a sequence of tabs into a group:
  //   Old:  Tab @ 0  |  Tab @ 1  |  Group @ 2
  //   New:  Group @ 0  |  Group @ 2
  //   -> Notify Tab 0 and Tab 1 closed, Group 0 inserted.
  //
  // Expand a group into a sequence of tabs:
  //   Old:  Group @ 0  |  Group @ 2
  //   New:  Tab @ 0  |  Tab @ 1  |  Group @ 2
  //   -> Notify Group 0 removed, Tab 0 and Tab 1 inserted

/*
  int model_index = 0;
  int old_item_index = 0;
  int new_item_index = 0;
  while (model_index < model_->count()) {
    // One of the two lists should have something starting at the position
    // we're currently processing 
    DCHECK(old_index >= items_.size() ||
           new_index >= new_items.size() ||
           items_[old_index].model_index() == model_index ||
           new_items[new_index].model_index() == model_index);
    if (items_[old_index].model_index() > model_index) {
      // Old list missing and entry for this, something was added.
      for (auto& observer : observers_)
        observer.ItemsInsertedAt(this, new_index, 1);
    }
  }
    */

  // As tabs are added, removed, and changed in the underlying model, the
  // items_ list will be updated and notifications will be issued. Therefore,
  // we know that the result of ComputeGroups will be the current list of
  // items, with either previous items collapsed into groups or previous groups
  // expanded into items. Notifications are issued assuming this is the case.

  /*


    while () {



      if () {
        // Old list has an entry for this, the new one doesn't, mark it removed.
        for (auto& observer : observers_)
          observer.ItemClosingAt(this, old_index);
      } else if () {
        // New list has an entry, old ones doesn't, mark it added.
        for (auto& observer : observers_)
          observer.ItemInsertedAt(this, new_index);
      } else if () {
        // Type was changed between a group and a tab at the same index. This
        // means a group was added or destroyed at this index. Mark the old one
        // closed and the new one inserted.
        for (auto& observer : observers_) {
          observer.ItemClosingAt(this, old_index);
          observer.ItemInsertedAt(this, new_index);
        }
      }

    }
    */
}

// This function computes the ideal set of groups given the current model. It
// does not replace the current groups or depend on it.
std::vector<TabStripGrouperItem> TabStripGrouper::ComputeGroups() const {
  std::vector<TabStripGrouperItem> items;
  items.reserve(model_->count());

  for (int i = 0; i < model_->count(); i++)
    items.emplace_back(model_->GetWebContentsAt(i), i);
  return items;
}

int TabStripGrouper::ItemIndexForModelIndex(int model_index) const {
  DCHECK(!items_.empty());
  auto lower = std::lower_bound(items_.begin(), items_.end(), model_index,
                                ItemModelIndexLess());

  // lower_bound will have returned wither a matching index or the next bigger
  // one. In the latter case, need to find the previous group.
  if (lower == items_.end() || lower->model_index() > model_index) {
    DCHECK(lower > items_.begin());
    --lower;
    DCHECK(lower->type() == TabStripGrouperItem::Type::GROUP);
  }
  return lower - items_.begin();
}

void TabStripGrouper::ShiftModelIndices(int starting_at, int delta) {
/*
  int first_item = ItemIndexForModelIndex(starting_at);
  for (int i = first_item; i < items_.size(); i++)
    items_[i].model_index_ += delta;
    */
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
                                   /*
  // If the deleted tab corresponds to a tab in 
  int item_index = ItemIndexForModelIndex(model_index);
  if (items_[item_index].type() == TabStripGrouperItem::Type::TAB) {
    DCHECK(items_[item_index].web_contents() == contents);

    for (auto& observer : observers_)
      observer->
  }
*/
  ShiftModelIndices(model_index, -1);
  Regroup();
}

void TabStripGrouper::TabDetachedAt(content::WebContents* contents, int model_index) {
  ShiftModelIndices(model_index, -1);
  Regroup();
}

void TabStripGrouper::TabDeactivated(content::WebContents* contents) {}

void TabStripGrouper::ActiveTabChanged(content::WebContents* old_contents,
                                       content::WebContents* new_contents,
                                       int index,
                                       int reason) {}

void TabStripGrouper::TabSelectionChanged(
    TabStripModel* tab_strip_model,
    const ui::ListSelectionModel& old_model) {}

void TabStripGrouper::TabMoved(content::WebContents* contents,
                               int from_index,
                               int to_index) {}

void TabStripGrouper::TabChangedAt(content::WebContents* contents,
                                   int index,
                                   TabChangeType change_type) {}

void TabStripGrouper::TabReplacedAt(TabStripModel* tab_strip_model,
                                    content::WebContents* old_contents,
                                    content::WebContents* new_contents,
                                    int index) {}

void TabStripGrouper::TabPinnedStateChanged(TabStripModel* tab_strip_model,
                                            content::WebContents* contents,
                                            int index) {}

void TabStripGrouper::TabBlockedStateChanged(content::WebContents* contents,
                                             int index) {}

void TabStripGrouper::TabStripEmpty() {}

void TabStripGrouper::WillCloseAllTabs() {}

void TabStripGrouper::CloseAllTabsCanceled() {}

void TabStripGrouper::TabNeedsAttentionAt(int index) {}
