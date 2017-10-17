// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_OFFLINE_PAGES_PREFETCH_PREFETCH_INTERNALS_DELEGATE_H_
#define CHROME_BROWSER_OFFLINE_PAGES_PREFETCH_PREFETCH_INTERNALS_DELEGATE_H_

namespace offline_pages {

// Class that pipes through //chrome -level dependencies of offline-internals.
class PrefetchInternalsDelegate {
 public:
  virtual ~PrefetchInternalsDelegate() = default;

  // Shows the prefetch notification with a test domain name so that it can be
  // inspected for UI issues.
  virtual void ShowNotificationForDebugging() = 0;
};

}  // namespace offline_pages

#endif  // CHROME_BROWSER_OFFLINE_PAGES_PREFETCH_PREFETCH_INTERNALS_DELEGATE_H_
