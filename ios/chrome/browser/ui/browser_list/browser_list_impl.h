// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_BROWSER_LIST_BROWSER_LIST_IMPL_H_
#define IOS_CHROME_BROWSER_UI_BROWSER_LIST_BROWSER_LIST_IMPL_H_

#include <memory>
#include <vector>

#include "base/macros.h"
#include "base/observer_list.h"
#include "ios/chrome/browser/ui/browser_list/browser.h"
#include "ios/chrome/browser/ui/browser_list/browser_list.h"

class BrowserListObserver;
class WebStateListDelegate;

namespace ios {
class ChromeBrowserState;
}

// BrowserListImpl implements the BrowserList interface.
class BrowserListImpl : public BrowserList {
 public:
  BrowserListImpl(ios::ChromeBrowserState* browser_state,
                  std::unique_ptr<WebStateListDelegate> delegate);

  ~BrowserListImpl() override;

  // Returns the number of Browsers in the BrowserListImpl.
  int count() const override;

  // Returns whether the specified index is valid.
  int ContainsIndex(int index) const override;

  // Returns the Browser at the specified index.
  Browser* GetBrowserAtIndex(int index) const override;

  // Returns the index of the specified Browser, or kInvalidIndex if not found.
  int GetIndexOfBrowser(const Browser* browser) const override;

  // Creates and returns a new Browser instance.
  Browser* CreateNewBrowser() override;

  // Closes the Browser at the specified index.
  void CloseBrowserAtIndex(int index) override;

  // Adds/removes |observer| from the list of observers.
  void AddObserver(BrowserListObserver* observer) override;
  void RemoveObserver(BrowserListObserver* observer) override;

 private:
  ios::ChromeBrowserState* browser_state_;
  std::unique_ptr<WebStateListDelegate> delegate_;
  std::vector<std::unique_ptr<Browser>> browsers_;
  base::ObserverList<BrowserListObserver, true> observers_;

  DISALLOW_COPY_AND_ASSIGN(BrowserListImpl);
};

#endif  // IOS_CHROME_BROWSER_UI_BROWSER_LIST_BROWSER_LIST_IMPL_H_
