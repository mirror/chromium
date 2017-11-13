// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_RESOURCE_COORDINATOR_BROWSER_ITERATOR_H_
#define CHROME_BROWSER_RESOURCE_COORDINATOR_BROWSER_ITERATOR_H_

#include <vector>

#include "base/macros.h"
#include "base/optional.h"

class Browser;
class TabStripModel;

namespace resource_coordinator {

// Information about a Browser.
struct BrowserInfo {
  const Browser* browser = nullptr;  // Can be nullptr in tests.
  TabStripModel* tab_strip_model = nullptr;
  bool window_is_minimized = false;
  bool browser_is_app = false;
};

// Interface for gathering a list of BrowserInfo objects for Browser windows.
class BrowserIterator {
 public:
  virtual ~BrowserIterator() = default;

  // Returns a list of BrowserInfo objects corresponding to Browser windows.
  virtual std::vector<BrowserInfo> GetBrowserInfoList() const = 0;

  // Returns the most recently active BrowserInfo, if any.
  virtual base::Optional<BrowserInfo> GetActiveBrowserInfo() const = 0;
};

class DefaultBrowserIterator : public BrowserIterator {
 public:
  DefaultBrowserIterator();
  ~DefaultBrowserIterator() override;

  // BrowserIterator:
  std::vector<BrowserInfo> GetBrowserInfoList() const override;
  base::Optional<BrowserInfo> GetActiveBrowserInfo() const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(DefaultBrowserIterator);
};

}  // namespace resource_coordinator

#endif  // CHROME_BROWSER_RESOURCE_COORDINATOR_BROWSER_ITERATOR_H_
