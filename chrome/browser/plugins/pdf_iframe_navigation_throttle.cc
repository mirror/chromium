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

// static
std::unique_ptr<content::NavigationThrottle>
PDFIFrameNavigationThrottle::MaybeCreateThrottleFor(
    content::NavigationHandle* handle) {
  if (handle->IsInMainFrame())
    return nullptr;

  content::WebPluginInfo pdf_plugin_info;
  ChromePluginServiceFilter::GetPdfPluginInfo(&pdf_plugin_info);

  ChromePluginServiceFilter* filter = ChromePluginServiceFilter::GetInstance();
  int process_id = handle->GetWebContents()->GetRenderProcessHost()->GetID();
  int routing_id =
      handle->GetWebContents()
          ->FindFrameByFrameTreeNodeId(handle->GetFrameTreeNodeId(), process_id)
          ->GetRoutingID();
  content::ResourceContext* resource_context =
      handle->GetWebContents()->GetBrowserContext()->GetResourceContext();
  if (filter->IsPluginAvailable(process_id, routing_id, resource_context,
                                handle->GetURL(), url::Origin(),
                                &pdf_plugin_info)) {
    return nullptr;
  }

  return base::MakeUnique<PDFIFrameNavigationThrottle>(handle);
}

content::NavigationThrottle::ThrottleCheckResult
PDFIFrameNavigationThrottle::WillProcessResponse() {
  std::string mime_type;
  navigation_handle()->GetResponseHeaders()->GetMimeType(&mime_type);
  if (mime_type != kPDFMimeType)
    return content::NavigationThrottle::PROCEED;

  // data:text/html,<body style="margin:0;"><object data="
  const char kPlaceholderDataUrlPrefix[] =
      "data:text/"
      "html,%3Cbody%20style%3D%22margin%3A0%3B%22%3E%3Cobject%20data%3D%22";
  // " type="application/pdf" style="width: 100%; height:
  // 100%;"></object></body>
  const char kPlaceholderDataUrlSuffix[] =
      "%22%20type%3D%22application%2Fpdf%22%20style%3D%22width%3A%20100%25%3B%"
      "20height%3A%20100%25%3B%22%3E%3C%2Fobject%3E%3C%2Fbody%3E";

  GURL data_url(kPlaceholderDataUrlPrefix +
                navigation_handle()->GetURL().spec() +
                kPlaceholderDataUrlSuffix);

  navigation_handle()->GetWebContents()->OpenURL(
      content::OpenURLParams(data_url, navigation_handle()->GetReferrer(),
                             navigation_handle()->GetFrameTreeNodeId(),
                             WindowOpenDisposition::CURRENT_TAB,
                             ui::PAGE_TRANSITION_AUTO_SUBFRAME, false));

  return content::NavigationThrottle::CANCEL_AND_IGNORE;
}
