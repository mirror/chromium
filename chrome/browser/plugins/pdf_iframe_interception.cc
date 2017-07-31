// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/plugins/pdf_iframe_interception.h"

#include "components/navigation_interception/intercept_navigation_throttle.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"

namespace {

bool InterceptNavigation(
    const GURL& url,
    const content::Referrer& referrer,
    int frame_tree_node_id,
    content::WebContents* source,
    const navigation_interception::NavigationParams& params) {
  GURL data_url("data:text/html,<body style='margin:0;'><object data='" +
                url.spec() +
                "' type='application/pdf' style='width: 100%; "
                "height: 100%;'></object></body>");

  source->OpenURL(content::OpenURLParams(data_url, referrer, frame_tree_node_id,
                                         WindowOpenDisposition::CURRENT_TAB,
                                         ui::PAGE_TRANSITION_AUTO_SUBFRAME,
                                         false));
  return true;
}

}  // namespace

std::unique_ptr<content::NavigationThrottle>
PDFIFrameInterception::MaybeCreateThrottleFor(
    content::NavigationHandle* handle) {
  if (handle->IsInMainFrame())
    return nullptr;

  // TODO(amberwon): Check that the URL is for a PDF and that no PDF Viewer is
  // available.

  return base::MakeUnique<navigation_interception::InterceptNavigationThrottle>(
      handle, base::Bind(&InterceptNavigation, handle->GetURL(),
                         handle->GetReferrer(), handle->GetFrameTreeNodeId()));
}
