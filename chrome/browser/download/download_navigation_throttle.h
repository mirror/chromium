// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_DOWNLOAD_DOWNLOAD_NAVIGATION_THROTTLE_H_
#define CHROME_BROWSER_DOWNLOAD_DOWNLOAD_NAVIGATION_THROTTLE_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "content/public/browser/navigation_throttle.h"

// DownloadNavigationThrottle is used to determine if a download should be
// allowed. It will check for storage permission and defer the request
// before it can be resumed.
// TODO(qinmin): add the throttling logic from DownloadRequestLimiter.
class DownloadNavigationThrottle : public content::NavigationThrottle {
 public:
  static std::unique_ptr<content::NavigationThrottle> Create(
      content::NavigationHandle* handle);
  ~DownloadNavigationThrottle() override;

  // content::NavigationThrottle:
  content::NavigationThrottle::ThrottleCheckResult WillProcessResponse()
  override;
  const char* GetNameForLogging() override;

 private:
  explicit DownloadNavigationThrottle(content::NavigationHandle* handle);

  void OnAcquireFileAccessPermissionDone(bool granted);

  // This has to be the last member of the class.
  base::WeakPtrFactory<DownloadNavigationThrottle> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(DownloadNavigationThrottle);
};

#endif  // CHROME_BROWSER_DOWNLOAD_DOWNLOAD_NAVIGATION_THROTTLE_H_
