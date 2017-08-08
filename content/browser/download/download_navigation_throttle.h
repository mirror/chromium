// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_DOWNLOAD_DOWNLOAD_NAVIGATION_THROTTLE_H_
#define CHROME_BROWSER_DOWNLOAD_DOWNLOAD_NAVIGATION_THROTTLE_H_

#include <memory>

#include "base/macros.h"
#include "content/public/browser/navigation_throttle.h"

namespace content {

// This class is responsible for handling all the download requests.
class DownloadNavigationThrottle : public NavigationThrottle {
 public:
  static std::unique_ptr<NavigationThrottle> CreateThrottleForNavigation(
      NavigationHandle* navigation_handle);

  ~DownloadNavigationThrottle() override;

  // NavigationThrottle method:
  ThrottleCheckResult WillProcessResponse() override;
  const char* GetNameForLogging() override;

 private:
  explicit DownloadNavigationThrottle(NavigationHandle* navigation_handle);
  DISALLOW_COPY_AND_ASSIGN(DownloadNavigationThrottle);
};

}  // namespace content

#endif  // CHROME_BROWSER_DOWNLOAD_DOWNLOAD_NAVIGATION_THROTTLE_H_
