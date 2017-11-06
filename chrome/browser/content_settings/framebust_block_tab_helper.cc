// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/content_settings/framebust_block_tab_helper.h"

#include "chrome/browser/content_settings/tab_specific_content_settings.h"
#include "content/public/browser/navigation_handle.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(FramebustBlockTabHelper);

FramebustBlockTabHelper::~FramebustBlockTabHelper() = default;

// static
void FramebustBlockTabHelper::AddBlockedUrl(const GURL& blocked_url) {
  blocked_urls_.push_back(blocked_url);

  TabSpecificContentSettings::FromWebContents(web_contents())
      ->OnContentBlocked(CONTENT_SETTINGS_TYPE_FRAMEBUST_BLOCK);

  if (observer_)
    observer_->OnBlockedUrlAdded(blocked_url);
}

const std::vector<GURL>& FramebustBlockTabHelper::GetBlockedUrls() {
  return blocked_urls_;
}

void FramebustBlockTabHelper::SetObserver(Observer* observer) {
  DCHECK(!observer_);
  observer_ = observer;
}

void FramebustBlockTabHelper::ClearObserver() {
  DCHECK(observer_);
  observer_ = nullptr;
}

void FramebustBlockTabHelper::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!navigation_handle->IsInMainFrame() || !navigation_handle->HasCommitted())
    return;

  blocked_urls_.clear();
}

FramebustBlockTabHelper::FramebustBlockTabHelper(
    content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents) {}
