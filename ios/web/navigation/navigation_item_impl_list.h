// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_NAVIGATION_NAVIGATION_ITEM_IMPL_LIST_H_
#define IOS_WEB_NAVIGATION_NAVIGATION_ITEM_IMPL_LIST_H_

#include <vector>

#include "base/memory/weak_ptr.h"
#import "ios/web/public/navigation_item_list.h"

namespace web {

class NavigationItemImpl;

// Concrete implementation of ScopedNavigationItemImplList.
class ScopedNavigationItemImplList : public ScopedNavigationItemList {
 public:
  // Constructor for a scoped list containing |items|.
  explicit ScopedNavigationItemImplList(
      const std::vector<std::unique_ptr<NavigationItemImpl>>& items);
  virtual ~ScopedNavigationItemImplList();

  // NavigationItemList:
  size_t size() const override;
  NavigationItem* GetItemAt(size_t index) const override;
  NavigationItem* operator [](size_t  index) const override;

  // ScopedNavigationItemList:
  std::unique_ptr<NavigationItem> GetScopedItemAtIndex(
      size_t index) override;

 private:
  // The list of scoped NavigationItemImpl pointers.
  std::vector<std::unique_ptr<NavigationItemImpl>> items_;
};

// Concrete implementation of WeakNavigationItemList.
class WeakNavigationItemImplList : public WeakNavigationItemList {
 public:
  // Constructor for a list containing weak references to |items|.
  explicit WeakNavigationItemImplList(
      const std::vector<NavigationItemImpl*>& items);
  // Constructor for a list containing weak references to the items in |list|.
  explicit WeakNavigationItemImplList(const NavigationItemList& list);
  virtual ~WeakNavigationItemImplList();

  // NavigationItemList:
  size_t size() const override;
  NavigationItem* GetItemAt(size_t index) const override;
  NavigationItem* operator [](size_t  index) const override;

 private:
  // The list of weak NavigationItemImpl pointers.
  std::vector<base::WeakPtr<NavigationItemImpl>> items_;
};

}  // namespace web

#endif  // IOS_WEB_NAVIGATION_NAVIGATION_ITEM_IMPL_LIST_H_
