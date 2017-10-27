// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SSL_SSL_ERROR_TAB_HELPER_H_
#define CHROME_BROWSER_SSL_SSL_ERROR_TAB_HELPER_H_

#include <map>
#include "components/security_interstitials/content/security_interstitial_page.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

namespace content {
class NavigationHandle;
class WebContents;
}  // namespace content

// Long-lived helper associated with a WebContents, for owning blocking pages.
class SSLErrorTabHelper
    : public content::WebContentsObserver,
      public content::WebContentsUserData<SSLErrorTabHelper> {
 public:
  ~SSLErrorTabHelper() override;

  // WebContentsObserver:
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;

  void SetBlockingPageForNavigation(
      int64_t navigation_id,
      std::unique_ptr<security_interstitials::SecurityInterstitialPage>
          blocking_page);

 private:
  explicit SSLErrorTabHelper(content::WebContents* web_contents);
  friend class content::WebContentsUserData<SSLErrorTabHelper>;

  // Keeps track of blocking pages for navigations that have encountered
  // certificate errors in this WebContents. When a navigation commits,
  // everything in this map is removed except the blocking page for the
  // currently committed navigation.
  std::map<int64_t,
           std::unique_ptr<security_interstitials::SecurityInterstitialPage>>
      blocking_pages_for_navigations_;
};

#endif  // CHROME_BROWSER_SSL_SSL_ERROR_TAB_HELPER_H_
