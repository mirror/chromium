// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/origin_manifest/origin_manifest_url_loader_throttle.h"

#include "base/logging.h"
#include "base/strings/string_util.h"
#include "chrome/browser/profiles/profile_io_data.h"
#include "content/public/browser/browser_context.h"
//#include "content/public/browser/browser_thread.h"
//#include "content/public/browser/resource_context.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/resource_response.h"
#include "net/http/http_request_headers.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_request_context_getter.h"
#include "url/origin.h"

namespace {

bool SanitizeOriginManifestVersion(std::string version, std::string* result) {
  if (!base::IsStringASCII(version))
    return false;

  base::TrimString(version, "\"", &version);

  // TODO(dhausknecht) is there a max length?
  if (version.length() == 0)
    return false;

  *result = version;
  return true;
}

}  // namespace

// static
std::unique_ptr<OriginManifestURLLoaderThrottle>
OriginManifestURLLoaderThrottle::Create(
    const base::Callback<content::WebContents*()>& wc_getter) {
  return std::unique_ptr<OriginManifestURLLoaderThrottle>(
      new OriginManifestURLLoaderThrottle(wc_getter));
}

OriginManifestURLLoaderThrottle::OriginManifestURLLoaderThrottle(
    const base::Callback<content::WebContents*()>& wc_getter)
    : web_contents_getter_(wc_getter) {}

OriginManifestURLLoaderThrottle::~OriginManifestURLLoaderThrottle() {}

void OriginManifestURLLoaderThrottle::WillStartRequest(
    const content::ResourceRequest& request,
    bool* defer) {
  origin_.reset(new url::Origin(request.url));
  *defer = false;
}

void OriginManifestURLLoaderThrottle::WillRedirectRequest(
    const net::RedirectInfo& redirect_info,
    bool* defer) {
  origin_.reset(new url::Origin(redirect_info.new_url));
  *defer = false;
}

void OriginManifestURLLoaderThrottle::WillProcessResponse(
    const content::ResourceResponseHead* response_head,
    bool* defer) {
  std::string version;
  if (response_head->headers.get() &&
      response_head->headers->EnumerateHeader(nullptr, "Sec-Origin-Manifest",
                                              &version)) {
    if (!SanitizeOriginManifestVersion(version, &version)) {
      *defer = false;
      return;
    }

    // TODO(dhausknecht) this way of fetching works for now But seems
    // non-optimal. I am sure there is a better way.
    content::WebContents* wc = web_contents_getter_.Run();
    content::StoragePartition* storage_partition =
        content::BrowserContext::GetStoragePartition(
            wc->GetBrowserContext(), wc->GetMainFrame()->GetSiteInstance());
    url_fetcher_ = net::URLFetcher::Create(
        GURL(origin_->GetURL().spec() + "originmanifest/" + version + ".json"),
        net::URLFetcher::GET, this);
    url_fetcher_->SetRequestContext(storage_partition->GetURLRequestContext());
    url_fetcher_->Start();

    *defer = true;
  }
  *defer = false;
}

void OriginManifestURLLoaderThrottle::OnURLFetchComplete(
    const net::URLFetcher* source) {
  std::string response_str;
  if (source->GetResponseCode() == 200 &&
      source->GetResponseAsString(&response_str)) {
    VLOG(1) << "Origin Manifest: " << response_str;
    // TODO(dhausknecht) parse and create the Origin Manifest. Then attach it to
    // the yet to be document / frame / ???
  }
  delegate_->Resume();
}
