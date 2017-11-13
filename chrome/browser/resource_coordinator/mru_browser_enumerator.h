// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_RESOURCE_COORDINATOR_MRU_BROWSER_ENUMERATOR_H_
#define CHROME_BROWSER_RESOURCE_COORDINATOR_MRU_BROWSER_ENUMERATOR_H_

#include "chrome/browser/resource_coordinator/browser_enumerator.h"

#include "base/macros.h"

namespace resource_coordinator {

// Provides a list of BrowserInfos sorted in most-recently-used order.
class MRUBrowserEnumerator : public BrowserEnumerator {
 public:
  MRUBrowserEnumerator();
  ~MRUBrowserEnumerator() override;

  // BrowserEnumerator:
  std::vector<BrowserInfo> GetBrowserInfoList() const override;
  base::Optional<BrowserInfo> GetActiveBrowserInfo() const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(MRUBrowserEnumerator);
};

}  // namespace resource_coordinator

#endif  // CHROME_BROWSER_RESOURCE_COORDINATOR_MRU_BROWSER_ENUMERATOR_H_
