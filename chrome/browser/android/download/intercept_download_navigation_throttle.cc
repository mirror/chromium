// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/download/intercept_download_navigation_throttle.h"

#include "base/bind.h"
#include "chrome/browser/android/download/download_controller_base.h"
#include "chrome/common/chrome_content_client.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"

using content::BrowserThread;

namespace {

static const char kOMADrmMessageMimeType[] = "application/vnd.oma.drm.message";
static const char kOMADrmContentMimeType[] = "application/vnd.oma.drm.content";
static const char kOMADrmRightsMimeType1[] =
    "application/vnd.oma.drm.rights+xml";
static const char kOMADrmRightsMimeType2[] =
    "application/vnd.oma.drm.rights+wbxml";

content::WebContents* GetWebContents(int render_process_id,
                                     int render_view_id) {
  content::RenderViewHost* render_view_host =
      content::RenderViewHost::FromID(render_process_id, render_view_id);

  if (!render_view_host)
    return nullptr;

  return content::WebContents::FromRenderViewHost(render_view_host);
}

}  // namespace

// static
std::unique_ptr<content::NavigationThrottle>
InterceptDownloadNavigationThrottle::Create(content::NavigationHandle* handle) {
  return base::WrapUnique(new InterceptDownloadNavigationThrottle(handle));
}

InterceptDownloadNavigationThrottle::~InterceptDownloadNavigationThrottle() =
    default;

content::NavigationThrottle::ThrottleCheckResult
InterceptDownloadNavigationThrottle::WillProcessResponse() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (!navigation_handle()->IsDownload())
    return content::NavigationThrottle::PROCEED;

  if (!navigation_handle()->GetURL().SchemeIsHTTPOrHTTPS())
    return content::NavigationThrottle::PROCEED;

  if (navigation_handle()->IsPost())
    return content::NavigationThrottle::PROCEED;

  const net::HttpResponseHeaders* headers =
      navigation_handle()->GetResponseHeaders();
  if (!headers)
    return content::NavigationThrottle::PROCEED;

  std::string mime_type;
  if (!headers->GetMimeType(&mime_type))
    return content::NavigationThrottle::PROCEED;

  if (!base::EqualsCaseInsensitiveASCII(mime_type, kOMADrmMessageMimeType) &&
      !base::EqualsCaseInsensitiveASCII(mime_type, kOMADrmContentMimeType) &&
      !base::EqualsCaseInsensitiveASCII(mime_type, kOMADrmRightsMimeType1) &&
      !base::EqualsCaseInsensitiveASCII(mime_type, kOMADrmRightsMimeType2)) {
    return content::NavigationThrottle::PROCEED;
  }

  InterceptDownload();
  return content::NavigationThrottle::CANCEL;
}

const char* InterceptDownloadNavigationThrottle::GetNameForLogging() {
  return "InterceptDownloadNavigationThrottle";
}

InterceptDownloadNavigationThrottle::InterceptDownloadNavigationThrottle(
    content::NavigationHandle* handle)
    : content::NavigationThrottle(handle) {}

void InterceptDownloadNavigationThrottle::InterceptDownload() {
  GURL original_url;
  const std::vector<GURL>& url_chain = navigation_handle()->GetRedirectChain();
  if (!url_chain.empty())
    original_url = url_chain.front();

  std::string content_disposition;
  std::string mime_type;
  const net::HttpResponseHeaders* headers =
      navigation_handle()->GetResponseHeaders();
  headers->GetMimeType(&mime_type);
  headers->GetNormalizedHeader("content-disposition", &content_disposition);
  content::WebContents* web_contents = navigation_handle()->GetWebContents();
  int process_id =
      web_contents ? web_contents->GetRenderViewHost()->GetProcess()->GetID()
                   : 0;
  int routing_id =
      web_contents ? web_contents->GetRenderViewHost()->GetRoutingID() : 0;

  DownloadControllerBase::Get()->CreateAndroidDownload(
      base::Bind(&GetWebContents, process_id, routing_id),
      DownloadInfo(navigation_handle()->GetURL(), original_url,
                   content_disposition, mime_type, GetUserAgent(),
                   // TODO(qinmin): Get the cookie from cookie store.
                   std::string(),
                   navigation_handle()->GetReferrer().url.spec()));
}
