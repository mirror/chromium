// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/frame_host/cross_site_document_blocking_throttle.h"

#include "content/browser/child_process_security_policy_impl.h"
#include "content/browser/loader/detachable_resource_handler.h"
#include "content/browser/loader/resource_request_info_impl.h"
#include "content/browser/site_instance_impl.h"
#include "content/common/cross_site_document_classifier.h"
#include "content/common/site_isolation_policy.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/resource_context.h"
#include "content/public/common/content_client.h"
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
  const GURL& url = request_->url();
  // Check if the response's site needs to have its documents blocked.
  // TODO(creis): This check can go away once the logic here is made fully
  // backward compatible, and we can enforce it always, regardless of Site
  // Isolation policy.
  auto* policy = ChildProcessSecurityPolicyImpl::GetInstance();
  if (!SiteIsolationPolicy::UseDedicatedProcessesForAllSites() &&
      !policy->IsIsolatedOrigin(url::Origin::Create(url)))
    return;

  // Only block if this is a request made from a renderer process.
  const ResourceRequestInfoImpl* info =
      ResourceRequestInfoImpl::ForRequest(request_);
  if (!info || info->GetChildID() == -1)
    return;

  // Only block documents from HTTP(S) schemes.
  if (!CrossSiteDocumentClassifier::IsBlockableScheme(url))
    return;

  DCHECK(request_->initiator().has_value());
  const url::Origin& initiator = request_->initiator().value();

  // Don't block same-site documents.
  if (CrossSiteDocumentClassifier::IsSameSite(initiator, url))
    return;

  // Look up MIME type.  If it doesn't claim to be a document, don't block it.
  std::string mime_type;
  request_->GetMimeType(&mime_type);
  CrossSiteDocumentMimeType canonical_mime_type =
      CrossSiteDocumentClassifier::GetCanonicalMimeType(mime_type);
  if (canonical_mime_type == CROSS_SITE_DOCUMENT_MIME_TYPE_OTHERS)
    return;

  // Allow the response through if it has valid CORS headers.
  std::string cors_header;
  request_->GetResponseHeaderByName("access-control-allow-origin",
                                    &cors_header);
  if (CrossSiteDocumentClassifier::IsValidCorsHeaderSet(initiator, url,
                                                        cors_header))
    return;

  // Give embedder a chance to skip document blocking for this response.
  if (GetContentClient()->browser()->ShouldBypassDocumentBlocking(initiator,
                                                                  url))
    return;

  DLOG(INFO) << "BLOCKING CROSS SITE DOCUMENT.  Initiator: " << initiator
             << ", url: " << url << ", type: " << resource_type_
             << ", mime_type: " << mime_type
             << ", process: " << info->GetChildID();

  // TODO(creis): Where can we sniff the data to decide to let it through?

  // Ensure that prefetch, etc, continue to cache the response, without sending
  // it to the renderer.
  if (info->detachable_handler())
    info->detachable_handler()->Detach();

  // TODO(creis): Return empty data via ResourceHandler::OnWillRead instead of
  // canceling the request.
  Cancel();
}

CrossSiteDocumentBlockingThrottle::CrossSiteDocumentBlockingThrottle(
    net::URLRequest* request,
    ResourceType resource_type)
    : request_(request), resource_type_(resource_type) {
  DCHECK(request_);
}

}  // namespace content
