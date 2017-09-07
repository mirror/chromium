// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/download/download_navigation_throttle.h"

#include "base/bind.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "build/build_config.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/navigation_handle.h"

#if defined(OS_ANDROID)
#include "chrome/browser/android/download/download_controller_base.h"
#include "content/public/browser/web_contents.h"
#endif

using content::BrowserThread;

// static
std::unique_ptr<content::NavigationThrottle> DownloadNavigationThrottle::Create(
    content::NavigationHandle* handle) {
  return base::WrapUnique(new DownloadNavigationThrottle(handle));
}

DownloadNavigationThrottle::~DownloadNavigationThrottle() {}

content::NavigationThrottle::ThrottleCheckResult
DownloadNavigationThrottle::WillProcessResponse() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (!navigation_handle()->IsDownload())
    return content::NavigationThrottle::PROCEED;

#if defined(OS_ANDROID)
  const content::ResourceRequestInfo::WebContentsGetter& web_contents_getter =
      base::Bind(&content::WebContents::FromFrameTreeNodeId,
          navigation_handle()->GetFrameTreeNodeId());
  DownloadControllerBase::Get()->AcquireFileAccessPermission(
      web_contents_getter, base::Bind(
          &DownloadNavigationThrottle::OnAcquireFileAccessPermissionDone,
          weak_ptr_factory_.GetWeakPtr()));
  return content::NavigationThrottle::DEFER;
#else
  return content::NavigationThrottle::PROCEED;
#endif
}

const char* DownloadNavigationThrottle::GetNameForLogging() {
  return "DownloadNavigationThrottle";
}

DownloadNavigationThrottle::DownloadNavigationThrottle(
    content::NavigationHandle* handle)
    : content::NavigationThrottle(handle),
      weak_ptr_factory_(this) {}

void DownloadNavigationThrottle::OnAcquireFileAccessPermissionDone(
    bool granted) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (granted)
    Resume();
  else
    CancelDeferredNavigation(content::NavigationThrottle::CANCEL_AND_IGNORE);
}
