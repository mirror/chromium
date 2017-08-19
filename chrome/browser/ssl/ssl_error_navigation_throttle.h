// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SSL_SSL_ERROR_NAVIGATION_THROTTLE_H_
#define CHROME_BROWSER_SSL_SSL_ERROR_NAVIGATION_THROTTLE_H_

#include <memory>
#include "content/public/browser/navigation_throttle.h"

namespace content {
class NavigationHandle;
}  // namespace content

class SSLErrorNavigationThrottle : public content::NavigationThrottle {
 public:
  explicit SSLErrorNavigationThrottle(content::NavigationHandle* handle);
  ~SSLErrorNavigationThrottle() override;

  // content::NavigationThrottle:
  ThrottleCheckResult WillFailRequest() override;
  const char* GetNameForLogging() override;
};

#endif  // CHROME_BROWSER_SSL_SSL_ERROR_NAVIGATION_THROTTLE_H_