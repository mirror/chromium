// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/app_list/chrome_app_list_item_list.h"

#include <utility>

#include "base/memory/ptr_util.h"
#include "chrome/browser/ui/app_list/chrome_app_list_item.h"

namespace app_list {

ChromeAppListItemList::ChromeAppListItemList() = default;

ChromeAppListItemList::~ChromeAppListItemList() = default;

ChromeAppListItem* ChromeAppListItemList::FindItem(const std::string& id) {
  for (const auto& item : app_list_items_) {
    if (item->id() == id)
      return item.get();
  }
  return nullptr;
}

bool ChromeAppListItemList::FindItemIndex(const std::string& id,
                                          size_t* index) {
  for (size_t i = 0; i < app_list_items_.size(); ++i) {
    if (app_list_items_[i]->id() == id) {
      *index = i;
      return true;
    }
  }
  return false;
}

ChromeAppListItem* ChromeAppListItemList::AddItem(
    std::unique_ptr<ChromeAppListItem> item_ptr) {
  ChromeAppListItem* item = item_ptr.get();
  CHECK(std::find_if(app_list_items_.cbegin(), app_list_items_.cend(),
                     [item](const std::unique_ptr<ChromeAppListItem>& item_p) {
                       return item_p.get() == item;
                     }) == app_list_items_.cend());
  app_list_items_.push_back(std::move(item_ptr));

  return item;
}

void ChromeAppListItemList::DeleteItem(const std::string& id) {
  std::unique_ptr<ChromeAppListItem> item = RemoveItem(id);
  // |item| will be deleted on destruction.
}

std::unique_ptr<ChromeAppListItem> ChromeAppListItemList::RemoveItem(
    const std::string& id) {
  size_t index;
  if (!FindItemIndex(id, &index))
    LOG(FATAL) << "RemoveItem: Not found: " << id;
  return RemoveItemAt(index);
}

std::unique_ptr<ChromeAppListItem> ChromeAppListItemList::RemoveItemAt(
    size_t index) {
  CHECK_LT(index, ItemCount());
  auto item = std::move(app_list_items_[index]);
  app_list_items_.erase(app_list_items_.begin() + index);
  return item;
}

void ChromeAppListItemList::DeleteItemAt(size_t index) {
  std::unique_ptr<ChromeAppListItem> item = RemoveItemAt(index);
  // |item| will be deleted on destruction.
}

}  // namespace app_list
