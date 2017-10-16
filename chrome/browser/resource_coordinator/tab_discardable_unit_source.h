// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_RESOURCE_COORDINATOR_TAB_DISCARDABLE_UNIT_SOURCE_H_
#define CHROME_BROWSER_RESOURCE_COORDINATOR_TAB_DISCARDABLE_UNIT_SOURCE_H_

#include "base/macros.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"

namespace resource_coordinator {

class TabManager;

class TabDiscardableUnitSource : public TabStripModelObserver,
                   public chrome::BrowserListObserver {
 public:
  TabDiscardableUnitSource(TabManager* tab_manager);
  ~TabDiscardableUnitSource();

 private:
  DISALLOW_COPY_AND_ASSIGN(TabDiscardableUnitSource);
};

}  // namespace resource_coordinator

#endif  // CHROME_BROWSER_RESOURCE_COORDINATOR_TAB_DISCARDABLE_UNIT_SOURCE_H_
