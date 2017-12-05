// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_TABS_BROWSER_ACTIVITY_WATCHER_H_
#define CHROME_BROWSER_UI_TABS_BROWSER_ACTIVITY_WATCHER_H_

#include <stddef.h>
#include <map>

#include "base/macros.h"
#include "base/scoped_observer.h"
#include "chrome/browser/ui/browser_list_observer.h"

class BrowserList;

// Observes browser activity in order to log BrowserMetrics UKMs for browser
// events relative to tab activation and discarding.
// Multiple tabs in the same browser can refer to the same BrowserMetrics entry.
// Must be used on the UI thread.
class BrowserActivityWatcher : public chrome::BrowserListObserver {
 public:
  // Represents a UKM entry for browser metrics.
  struct BrowserMetrics;

  // Returns the single instance, creating it if necessary.
  static BrowserActivityWatcher* GetInstance();

  // Returns the browser ID for a UKM BrowserMetrics entry reflecting the
  // browser's current stats.
  // A new UKM entry will only be logged if an existing entry for the same
  // browser doesn't exist yet or is stale.
  uint64_t CreateOrUpdateBrowserMetrics(const Browser* browser);

 private:
  BrowserActivityWatcher();
  ~BrowserActivityWatcher() override;

  // BrowserListObserver:
  void OnBrowserRemoved(Browser* browser) override;
  void OnBrowserSetLastActive(Browser* browser) override;
  void OnBrowserNoLongerActive(Browser* browser) override;

  ScopedObserver<BrowserList, BrowserListObserver> browser_list_observer_;

  // Cache of BrowserMetrics logged for each browser. Used to avoid logging
  // duplicate identical UKM events.
  std::map<const Browser*, BrowserMetrics> browser_metrics_;

  DISALLOW_COPY_AND_ASSIGN(BrowserActivityWatcher);
};

#endif  // CHROME_BROWSER_UI_TABS_BROWSER_ACTIVITY_WATCHER_H_
