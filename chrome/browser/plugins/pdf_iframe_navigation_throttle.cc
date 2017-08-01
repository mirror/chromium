// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/plugins/pdf_iframe_navigation_throttle.h"

#include "chrome/browser/plugins/chrome_plugin_service_filter.h"
#include "chrome/common/pdf_uma.h"
#include "chrome/common/render_messages.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"
#include "net/http/http_response_headers.h"

PDFIFrameNavigationThrottle::PDFIFrameNavigationThrottle(
    content::NavigationHandle* navigation_handle)
    : content::NavigationThrottle(navigation_handle) {}

PDFIFrameNavigationThrottle::~PDFIFrameNavigationThrottle() {}

const char* PDFIFrameNavigationThrottle::GetNameForLogging() {
  return "PDFIFrameNavigationThrottle";
}

void PDFIFrameNavigationThrottle::InterceptNavigation() {
  GURL data_url("data:text/html,<body style='margin:0;'><object data='" +
                navigation_handle()->GetURL().spec() +
                "' type='application/pdf' style='width: 100%; "
                "height: 100%;'></object></body>");

  navigation_handle()->GetWebContents()->OpenURL(
      content::OpenURLParams(data_url, navigation_handle()->GetReferrer(),
                             navigation_handle()->GetFrameTreeNodeId(),
                             WindowOpenDisposition::CURRENT_TAB,
                             ui::PAGE_TRANSITION_AUTO_SUBFRAME, false));
}

// static
std::unique_ptr<content::NavigationThrottle>
PDFIFrameNavigationThrottle::MaybeCreateThrottleFor(
    content::NavigationHandle* handle) {
  if (handle->IsInMainFrame())
    return nullptr;

  return base::MakeUnique<PDFIFrameNavigationThrottle>(handle);
}

content::NavigationThrottle::ThrottleCheckResult
PDFIFrameNavigationThrottle::WillProcessResponse() {
  std::string mime_type;
  navigation_handle()->GetResponseHeaders()->GetMimeType(&mime_type);
  if (mime_type != kPDFMimeType)
    return content::NavigationThrottle::PROCEED;

  content::WebPluginInfo pdf_plugin_info;
  ChromePluginServiceFilter::GetPdfPluginInfo(&pdf_plugin_info);

  ChromePluginServiceFilter* filter = ChromePluginServiceFilter::GetInstance();
  if (filter->IsPluginAvailable(
          navigation_handle()->GetRenderFrameHost()->GetProcess()->GetID(),
          navigation_handle()->GetRenderFrameHost()->GetRoutingID(),
          navigation_handle()
              ->GetWebContents()
              ->GetBrowserContext()
              ->GetResourceContext(),
          navigation_handle()->GetURL(), url::Origin(), &pdf_plugin_info))
    return content::NavigationThrottle::PROCEED;

  InterceptNavigation();
  return content::NavigationThrottle::CANCEL_AND_IGNORE;
}
