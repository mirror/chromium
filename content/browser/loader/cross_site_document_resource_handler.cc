// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/cross_site_document_resource_handler.h"

#include "base/trace_event/trace_event.h"
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

  // Have the downstream handler(s) allocate the real buffer to use.
  next_handler_->OnWillRead(buf, buf_size, std::move(controller));

  // If we intend to block the response, make the buffer look minimally small so
  // that almost no data is read into it, since the buffer is visible to the
  // renderer process. A zero size buffer isn't supported.
  // TODO(creis): If we add content sniffing to verify the MIME type, read the
  // data into a different buffer and copy it over if we decide to allow it.
  if (should_block_response_)
    *buf_size = 1;
}

void CrossSiteDocumentResourceHandler::OnReadCompleted(
    int bytes_read,
    std::unique_ptr<ResourceController> controller) {
  // When blocking the response, tell downstream handlers that no bytes were
  // received to appear as if the response is complete. We will leak at most one
  // byte in the shared buffer (see comment in OnWillRead).
  if (should_block_response_) {
    DCHECK_LE(bytes_read, 1);
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

  // Allow requests from file:// URLs for now.
  // TODO(creis): Limit this to when the allow_universal_access_from_file_urls
  // preference is set.  See https://crbug.com/789781.
  if (initiator.scheme() == url::kFileScheme)
    return false;

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

  // TODO(creis): Don't block plugin requests with universal access. This could
  // be done by allowing resource_type_ == RESOURCE_TYPE_PLUGIN_RESOURCE unless
  // it had an Origin request header and IsValidCorsHeaderSet returned false.
  // (That would matter for plugins without universal access, which use CORS.)

  // TODO(creis): Sniff the data to verify that it actually looks like the type
  // it claims to be, to avoid breaking mislabeled scripts, etc.

  // Give embedder a chance to skip document blocking for this response.
  if (GetContentClient()->browser()->ShouldBypassDocumentBlocking(initiator,
                                                                  url))
    return false;

  DLOG(INFO) << "BLOCKING CROSS SITE DOCUMENT.  Initiator: " << initiator
             << ", url: " << url << ", type: " << resource_type_
             << ", mime_type: " << mime_type
             << ", process: " << info->GetChildID();

  TRACE_EVENT2(
      "navigation", "CrossSiteDocumentResourceHandler::ShouldBlockResponse",
      "initiator", initiator.Serialize(), "url", url.possibly_invalid_spec());

  return true;
}

}  // namespace content
