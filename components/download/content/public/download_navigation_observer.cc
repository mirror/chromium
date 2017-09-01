// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/content/public/download_navigation_observer.h"

#include "base/memory/ptr_util.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(download::DownloadNavigationObserver);

namespace download {

// static
void DownloadNavigationObserver::CreateForWebContents(
    content::WebContents* web_contents,
    NavigationMonitor* navigation_monitor) {
  DCHECK(web_contents);
  if (!FromWebContents(web_contents)) {
    web_contents->SetUserData(UserDataKey(),
                              base::MakeUnique<DownloadNavigationObserver>(
                                  web_contents, navigation_monitor));
  }
}

DownloadNavigationObserver::DownloadNavigationObserver(
    content::WebContents* web_contents,
    NavigationMonitor* navigation_monitor)
    : content::WebContentsObserver(web_contents),
      navigation_monitor_(navigation_monitor) {}

DownloadNavigationObserver::~DownloadNavigationObserver() = default;

void DownloadNavigationObserver::DidStartLoading() {
  NotifyNavigationEvent();
}

void DownloadNavigationObserver::DidStopLoading() {
  NotifyNavigationEvent();
}

void DownloadNavigationObserver::NotifyNavigationEvent() {
  DCHECK(navigation_monitor_);
  navigation_monitor_->OnNavigationEvent();
}

}  // namespace download
