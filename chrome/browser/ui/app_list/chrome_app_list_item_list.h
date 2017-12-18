// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_APP_LIST_CHROME_APP_LIST_ITEM_LIST_H_
#define CHROME_BROWSER_UI_APP_LIST_CHROME_APP_LIST_ITEM_LIST_H_

#include <stddef.h>

#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/observer_list.h"
#include "components/sync/model/string_ordinal.h"

class ChromeAppListItem;

namespace app_list {

// Class to manage chrome app list items, used by ChromeAppListModelUpdater.
class ChromeAppListItemList {
 public:
  ChromeAppListItemList();
  virtual ~ChromeAppListItemList();

  // Finds item matching |id|. NOTE: Requires a linear search.
  // Generally this should not be used directly, AppListModel::FindItem
  // should be used instead.
  ChromeAppListItem* FindItem(const std::string& id);

  // Finds the |index| of the the item matching |id| in |app_list_items_|.
  // Returns true if the matching item is found.
  // Note: Requires a linear search.
  bool FindItemIndex(const std::string& id, size_t* index);

  ChromeAppListItem* ItemAt(size_t index) {
    DCHECK_LT(index, app_list_items_.size());
    return app_list_items_[index].get();
  }
  const ChromeAppListItem* ItemAt(size_t index) const {
    DCHECK_LT(index, app_list_items_.size());
    return app_list_items_[index].get();
  }
  size_t ItemCount() const { return app_list_items_.size(); }

  // Adds |item| to the end of |app_list_items_|. Takes ownership of |item|.
  // Triggers observers_.OnListItemAdded(). Returns a pointer to the added item
  // that is safe to use (e.g. after releasing a scoped ptr).
  ChromeAppListItem* AddItem(std::unique_ptr<ChromeAppListItem> item_ptr);

  // Finds item matching |id| in |app_list_items_| (linear search) and deletes
  // it. Triggers observers_.OnListItemRemoved() after removing the item from
  // the list and before deleting it.
  void DeleteItem(const std::string& id);

 private:
  // Removes the item with matching |id| in |app_list_items_| without deleting
  // it. Returns a scoped pointer containing the removed item.
  std::unique_ptr<ChromeAppListItem> RemoveItem(const std::string& id);

  // Removes the item at |index| from |app_list_items_| without deleting it.
  // Returns a scoped pointer containing the removed item.
  std::unique_ptr<ChromeAppListItem> RemoveItemAt(size_t index);

  // Deletes item at |index| and signals observers.
  void DeleteItemAt(size_t index);

  std::vector<std::unique_ptr<ChromeAppListItem>> app_list_items_;

  DISALLOW_COPY_AND_ASSIGN(ChromeAppListItemList);
};

}  // namespace app_list

#endif  // CHROME_BROWSER_UI_APP_LIST_CHROME_APP_LIST_ITEM_LIST_H_
