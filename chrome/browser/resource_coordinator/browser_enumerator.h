// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_RESOURCE_COORDINATOR_BROWSER_ENUMERATOR_H_
#define CHROME_BROWSER_RESOURCE_COORDINATOR_BROWSER_ENUMERATOR_H_

#include <vector>

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
class BrowserEnumerator {
 public:
  virtual ~BrowserEnumerator() = default;

  // Returns a list of BrowserInfo objects corresponding to Browser windows.
  virtual std::vector<BrowserInfo> GetBrowserInfoList() const = 0;

  // Returns a BrowserInfo for the currently active Browser, if any.
  virtual base::Optional<BrowserInfo> GetActiveBrowserInfo() const = 0;
};

}  // namespace resource_coordinator

#endif  // CHROME_BROWSER_RESOURCE_COORDINATOR_BROWSER_ENUMERATOR_H_
