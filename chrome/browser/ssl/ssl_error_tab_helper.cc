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
  if (!navigation_handle->HasCommitted() ||
      navigation_handle->IsSameDocument()) {
    return;
  }

  int64_t navigation_id = navigation_handle->GetNavigationId();

  auto it = blocking_pages_for_navigations_.begin();
  while (it != blocking_pages_for_navigations_.end()) {
    if (it->first == navigation_id) {
      ++it;
    } else {
      it = blocking_pages_for_navigations_.erase(it);
    }
  }
}

void SSLErrorTabHelper::SetBlockingPageForNavigation(
    int64_t navigation_id,
    std::unique_ptr<security_interstitials::SecurityInterstitialPage>
        blocking_page) {
  blocking_pages_for_navigations_[navigation_id] = std::move(blocking_page);
}
