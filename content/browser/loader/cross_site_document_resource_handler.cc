// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/cross_site_document_resource_handler.h"

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

CrossSiteDocumentResourceHandler::CrossSiteDocumentResourceHandler(
    std::unique_ptr<ResourceHandler> next_handler,
    net::URLRequest* request,
    ResourceType resource_type)
    : LayeredResourceHandler(request, std::move(next_handler)),
      resource_type_(resource_type),
      should_block_response_(false),
      blocked_read_completed_(false) {}

CrossSiteDocumentResourceHandler::~CrossSiteDocumentResourceHandler() {}

void CrossSiteDocumentResourceHandler::OnResponseStarted(
    ResourceResponse* response,
    std::unique_ptr<ResourceController> controller) {
  should_block_response_ = ShouldBlockResponse();
  next_handler_->OnResponseStarted(response, std::move(controller));
}

void CrossSiteDocumentResourceHandler::OnWillRead(
    scoped_refptr<net::IOBuffer>* buf,
    int* buf_size,
    std::unique_ptr<ResourceController> controller) {
  if (blocked_read_completed_) {
    // Ensure that prefetch, etc, continue to cache the response, without
    // sending it to the renderer.
    const ResourceRequestInfoImpl* info =
        ResourceRequestInfoImpl::ForRequest(request());
    if (info && info->detachable_handler()) {
      info->detachable_handler()->Detach();
      // TODO(creis): Do we need to ensure OnResponseCompleted is called?
    } else {
      // If it's not detachable, cancel the rest of the request.
      controller->Cancel();
    }
    return;
  }

  next_handler_->OnWillRead(buf, buf_size, std::move(controller));
}

void CrossSiteDocumentResourceHandler::OnReadCompleted(
    int bytes_read,
    std::unique_ptr<ResourceController> controller) {
  // When blocking the response, tell downstream handlers that no bytes were
  // received.
  if (should_block_response_) {
    bytes_read = 0;
    blocked_read_completed_ = true;
  }
  next_handler_->OnReadCompleted(bytes_read, std::move(controller));
}

void CrossSiteDocumentResourceHandler::OnResponseCompleted(
    const net::URLRequestStatus& status,
    std::unique_ptr<ResourceController> controller) {
  if (blocked_read_completed_) {
    // Report blocked responses as successful.
    next_handler_->OnResponseCompleted(net::URLRequestStatus(),
                                       std::move(controller));
  } else {
    next_handler_->OnResponseCompleted(status, std::move(controller));
  }
}

bool CrossSiteDocumentResourceHandler::ShouldBlockResponse() {
  const GURL& url = request()->url();
  // Check if the response's site needs to have its documents blocked.
  // TODO(creis): This check can go away once the logic here is made fully
  // backward compatible, and we can enforce it always, regardless of Site
  // Isolation policy.
  auto* policy = ChildProcessSecurityPolicyImpl::GetInstance();
  if (!SiteIsolationPolicy::UseDedicatedProcessesForAllSites() &&
      !policy->IsIsolatedOrigin(url::Origin::Create(url)))
    return false;

  // Only block if this is a request made from a renderer process.
  const ResourceRequestInfoImpl* info =
      ResourceRequestInfoImpl::ForRequest(request());
  if (!info || info->GetChildID() == -1)
    return false;

  // Only block documents from HTTP(S) schemes.
  if (!CrossSiteDocumentClassifier::IsBlockableScheme(url))
    return false;

  DCHECK(request()->initiator().has_value());
  const url::Origin& initiator = request()->initiator().value();

  // Don't block same-site documents.
  if (CrossSiteDocumentClassifier::IsSameSite(initiator, url))
    return false;

  // Look up MIME type.  If it doesn't claim to be a document, don't block it.
  std::string mime_type;
  request()->GetMimeType(&mime_type);
  CrossSiteDocumentMimeType canonical_mime_type =
      CrossSiteDocumentClassifier::GetCanonicalMimeType(mime_type);
  if (canonical_mime_type == CROSS_SITE_DOCUMENT_MIME_TYPE_OTHERS)
    return false;

  // Allow the response through if it has valid CORS headers.
  std::string cors_header;
  request()->GetResponseHeaderByName("access-control-allow-origin",
                                     &cors_header);
  if (CrossSiteDocumentClassifier::IsValidCorsHeaderSet(initiator, url,
                                                        cors_header))
    return false;

  // TODO(creis): Don't block plugin requests with universal access.

  // Give embedder a chance to skip document blocking for this response.
  if (GetContentClient()->browser()->ShouldBypassDocumentBlocking(initiator,
                                                                  url))
    return false;

  // TODO(creis): Where can we sniff the data to decide to let it through?

  DLOG(INFO) << "BLOCKING CROSS SITE DOCUMENT.  Initiator: " << initiator
             << ", url: " << url << ", type: " << resource_type_
             << ", mime_type: " << mime_type
             << ", process: " << info->GetChildID();
  return true;
}

}  // namespace content
