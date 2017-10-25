// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ssl/ssl_error_tab_helper.h"

#include "content/public/browser/navigation_handle.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(SSLErrorTabHelper);

SSLErrorTabHelper::SSLErrorTabHelper(content::WebContents* web_contents) {
  Observe(web_contents);
}

SSLErrorTabHelper::~SSLErrorTabHelper() {}

void SSLErrorTabHelper::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  int64_t navigation_id = navigation_handle->GetNavigationId();
  std::unique_ptr<security_interstitials::SecurityInterstitialPage> current =
      std::move(blocking_pages_for_navigations_[navigation_id]);
  blocking_pages_for_navigations_.clear();
  blocking_pages_for_navigations_[navigation_id] = std::move(current);
}

void SSLErrorTabHelper::SetBlockingPageForNavigation(
    std::unique_ptr<security_interstitials::SecurityInterstitialPage>
        blocking_page,
    int64_t navigation_id) {
  blocking_pages_for_navigations_[navigation_id] = std::move(blocking_page);
}
