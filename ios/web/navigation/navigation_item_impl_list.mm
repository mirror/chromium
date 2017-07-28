// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/navigation/navigation_item_impl_list.h"

#include "base/memory/ptr_util.h"
#import "ios/web/navigation/navigation_item_impl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace web {

#pragma mark - ScopedNavigationItemImplList

ScopedNavigationItemImplList::ScopedNavigationItemImplList(
    const std::vector<std::unique_ptr<NavigationItemImpl>>& items)
    : items_(std::move(items)) {}

ScopedNavigationItemImplList::~ScopedNavigationItemImplList() {}

size_t ScopedNavigationItemImplList::size() const { return items_.size(); }

NavigationItem* ScopedNavigationItemImplList::GetItemAt(size_t index) const {
  if (index >= size())
    return nullptr;
  return items_[index].get();
}

NavigationItem* ScopedNavigationItemImplList::operator [](size_t  index) const {
  return GetItemAt(index);
}

std::unique_ptr<NavigationItem>
ScopedNavigationItemImplList::GetScopedItemAtIndex(size_t index) {
  if (index >= size())
    return std::unique_ptr<NavigationItem>();
  return std::move(items_[index]);
}

#pragma mark - WeakNavigationItemImplList

// static
std::unique_ptr<WeakNavigationItemList>
WeakNavigationItemList::FromNavigationItemList(
    const NavigationItemList& scoped_list) {
  return base::MakeUnique<WeakNavigationItemImplList>(scoped_list);
}

WeakNavigationItemImplList::WeakNavigationItemImplList(
    const std::vector<NavigationItemImpl*>& items)
    : items_(items.size()) {
  for (size_t i = 0; i < items.size(); ++i) {
    items_[i] = items[i]->GetWeakPtr();
  }
}

WeakNavigationItemImplList::WeakNavigationItemImplList(
    const NavigationItemList& list)
    : items_(list.size()) {
  for (size_t i = 0; i < list.size(); ++i) {
    items_[i] = static_cast<NavigationItemImpl*>(list[i])->GetWeakPtr();
  }
}

WeakNavigationItemImplList::~WeakNavigationItemImplList() {}

size_t WeakNavigationItemImplList::size() const { return items_.size(); }

NavigationItem* WeakNavigationItemImplList::GetItemAt(size_t index) const {
  if (index >= size())
    return nullptr;
  return items_[index].get();
}

NavigationItem* WeakNavigationItemImplList::operator [](size_t  index) const {
  return GetItemAt(index);
}

}  // namespace web
