// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/frame_host/cross_site_document_blocking_throttle.h"

#include "content/browser/site_instance_impl.h"
#include "net/url_request/url_request.h"

namespace content {

namespace {

const char kNameForLogging[] = "CrossSiteDocumentBlockingThrottle";

}  // namespace

// static
std::unique_ptr<ResourceThrottle>
CrossSiteDocumentBlockingThrottle::MaybeCreateThrottleForRequest(
    net::URLRequest* request,
    ResourceType resource_type) {
  // No need to create this throttle for navigations, which will go into an
  // appropriate process.
  if (IsResourceTypeFrame(resource_type))
    return nullptr;

  return base::WrapUnique(
      new CrossSiteDocumentBlockingThrottle(request, resource_type));
}

CrossSiteDocumentBlockingThrottle::~CrossSiteDocumentBlockingThrottle() {}

const char* CrossSiteDocumentBlockingThrottle::GetNameForLogging() const {
  return kNameForLogging;
}

void CrossSiteDocumentBlockingThrottle::WillProcessResponse(bool* defer) {
  DLOG(INFO) << "XSDB: WillProcessResponse for " << request_->url()
             << ", type: " << resource_type_;

  // TODO(creis): Is initiator accurate enough?
  DCHECK(request_->initiator().has_value());
  const GURL& initiator = request_->initiator()->GetURL();

  // TODO(creis): Is this the right check, vs CanAccessDataForOrigin, etc?
  if (SiteInstance::IsSameWebSite(nullptr, initiator, request_->url()))
    return;

  // TODO(creis): Get MIME type from response.  Look at sniffing result?
  bool is_document = true;
  if (!is_document)
    return;

  if (resource_type_ == RESOURCE_TYPE_IMAGE ||
      resource_type_ == RESOURCE_TYPE_STYLESHEET) {
    DLOG(INFO) << "BLOCKING CROSS SITE DOCUMENT.  Initiator: " << initiator
               << ", url: " << request_->url() << ", type: " << resource_type_;

    // TODO(creis): How do we return an empty response instead, allowing the
    // full response to end up in the cache (e.g., for prefetch)?
    Cancel();
  }
}

CrossSiteDocumentBlockingThrottle::CrossSiteDocumentBlockingThrottle(
    net::URLRequest* request,
    ResourceType resource_type)
    : request_(request), resource_type_(resource_type) {
  DCHECK(request_);
}

}  // namespace content
