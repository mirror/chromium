// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_CHROME_MAIN_DELEGATE_STAGING_ANDROID_H_
#define CHROME_BROWSER_ANDROID_CHROME_MAIN_DELEGATE_STAGING_ANDROID_H_

#include "chrome/app/android/chrome_main_delegate_android.h"

#if defined(SAFE_BROWSING_SERVICE)
#include "chrome/browser/android/spdy_proxy_resource_throttle.h"
#endif

class SpdyProxyResourceThrottleFactory;

class ChromeMainDelegateStagingAndroid : public ChromeMainDelegateAndroid {
 public:
  ChromeMainDelegateStagingAndroid();
  ~ChromeMainDelegateStagingAndroid() override;

 protected:
  int RunProcess(
      const std::string& process_type,
      const content::MainFunctionParams& main_function_params) override;

 private:
  bool BasicStartupComplete(int* exit_code) override;
  void ProcessExiting(const std::string& process_type) override;
#if defined(SAFE_BROWSING_SERVICE)
  scoped_ptr<SpdyProxyResourceThrottleFactory> spdy_proxy_throttle_factory_;
#endif

  DISALLOW_COPY_AND_ASSIGN(ChromeMainDelegateStagingAndroid);
};

#endif  // CHROME_BROWSER_ANDROID_CHROME_MAIN_DELEGATE_STAGING_ANDROID_H_
