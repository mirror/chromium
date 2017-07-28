// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_PUBLIC_NAVIGATION_ITEM_LIST_H_
#define IOS_WEB_PUBLIC_NAVIGATION_ITEM_LIST_H_

#include <cstddef>
#include <memory>

namespace web {

class NavigationItem;

// A class representing a list of NavigationItems.
class NavigationItemList {
 public:
  // Returns whether the NavigationItemList is empty.
  bool empty() const { return !size(); }

  // Returns the number of NavigationItems pointers in the list.
  virtual size_t size() const = 0;

  // Returns the NavigationItem at |index|.
  virtual NavigationItem* GetItemAt(size_t index) const = 0;

  // This class supports square bracket notation, which has the same behavior as
  // GetItemAt().
  virtual NavigationItem* operator [](size_t  index) const = 0;
};

// A NavigationItemList that owns the pointers to its NavigationItems.
class ScopedNavigationItemList : public NavigationItemList {
 public:
  // Returns the scoped pointer at |index|, transferring ownership of that
  // NavigationItem to the caller.
  virtual std::unique_ptr<NavigationItem> GetScopedItemAtIndex(
      size_t index) = 0;
};

// A NavigationItemList containing weak references to NavigationItems.
class WeakNavigationItemList : public NavigationItemList {
 public:
  // Factory method that creates weak references from the scoped NavigationItem
  // poitners in |scoped_list|.
  static std::unique_ptr<WeakNavigationItemList> FromNavigationItemList(
      const NavigationItemList& scoped_list);
};

}  // namespace web

#endif  // IOS_WEB_PUBLIC_NAVIGATION_ITEM_LIST_H_
