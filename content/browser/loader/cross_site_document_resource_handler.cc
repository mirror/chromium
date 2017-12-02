// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/cross_site_document_resource_handler.h"

#include "base/trace_event/trace_event.h"
#include "content/browser/child_process_security_policy_impl.h"
#include "content/browser/loader/detachable_resource_handler.h"
#include "content/browser/loader/resource_request_info_impl.h"
#include "content/browser/site_instance_impl.h"
#include "content/common/site_isolation_policy.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/resource_context.h"
#include "content/public/common/content_client.h"
#include "net/base/io_buffer.h"
#include "net/url_request/url_request.h"

namespace content {

CrossSiteDocumentResourceHandler::CrossSiteDocumentResourceHandler(
    std::unique_ptr<ResourceHandler> next_handler,
    net::URLRequest* request,
    ResourceType resource_type)
    : LayeredResourceHandler(request, std::move(next_handler)),
      resource_type_(resource_type),
      next_handler_buffer_size_(0),
      should_block_response_(false),
      canonical_mime_type_(CROSS_SITE_DOCUMENT_MIME_TYPE_OTHERS),
      no_sniff_(false),
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
    // On the next read attempt after the response was blocked, either cancel
    // the rest of the request or allow it to proceed in a detached state.
    const ResourceRequestInfoImpl* info =
        ResourceRequestInfoImpl::ForRequest(request());
    if (info && info->detachable_handler()) {
      // Ensure that prefetch, etc, continue to cache the response, without
      // sending it to the renderer.
      info->detachable_handler()->Detach();
    } else {
      // If it's not detachable, cancel the rest of the request.
      controller->Cancel();
    }
    return;
  }

  // Have the downstream handler(s) allocate the real buffer to use.
  next_handler_->OnWillRead(buf, buf_size, std::move(controller));

  // If we intend to block the response, don't read into the buffer provided by
  // the downstream handler(s).
  if (should_block_response_) {
    if (no_sniff_) {
      // There's no need to sniff any of the data, so prepare to send the
      // renderer an empty response.  This is better than failing the request,
      // which changes page-visible behavior (e.g., for a blocked XHR).  To send
      // an empty response, first prepare a throwaway buffer (not shared with
      // the renderer) to read a single byte from the stream, since providing a
      // zero size buffer here isn't supported.  We will later complete the
      // response in OnReadCompleted and cancel any further reads in the next
      // call to OnWillRead.
      // TODO: Write into a junk buffer here as well.
      local_buffer_ = new net::IOBuffer(static_cast<size_t>(1));
      *buf = local_buffer_;
      *buf_size = 1;
      return;
    }

    // For most blocked responses, we need to sniff the data to confirm it looks
    // like the claimed MIME type (to avoid blocking mislabeled JavaScript,
    // JSONP, etc).  Read this data into a separate buffer (not shared with the
    // renderer), which we will only copy over if we decide to allow it through.
    // Make it as big as the downstream handler's buffer to make it easy to copy
    // over in one operation.
    // This will be large, since the MIME sniffing handler is downstream.
    DCHECK_GE(*buf_size, 200);
    local_buffer_ = new net::IOBuffer(static_cast<size_t>(*buf_size));

    // Store the next handler's buffer but don't read into it while sniffing,
    // since we probably won't want to send the data to the renderer process.
    next_handler_buffer_ = *buf;
    next_handler_buffer_size_ = *buf_size;
    *buf = local_buffer_;
  }
}

void CrossSiteDocumentResourceHandler::OnReadCompleted(
    int bytes_read,
    std::unique_ptr<ResourceController> controller) {
  // If we intended to block the response, do content sniffing if needed and
  // then report that zero bytes were read, indicating the response is complete.
  // If sniffing fails, we may need to copy the sniffed data over and resume the
  // response without blocking.
  if (should_block_response_) {
    bool mime_type_confirmed = false;
    if (no_sniff_) {
      mime_type_confirmed = true;
    } else {
      // Sniff the data to see if it likely matches the MIME type that caused us
      // to decide to block it.  If it doesn't match, it may be JavaScript,
      // JSONP, or another allowable data type and we should let it through.
      DCHECK_LE(bytes_read, next_handler_buffer_size_);
      base::StringPiece data(local_buffer_->data(), bytes_read);

      // Confirm whether the data is HTML, XML, or JSON.
      if (canonical_mime_type_ == CROSS_SITE_DOCUMENT_MIME_TYPE_HTML) {
        mime_type_confirmed = CrossSiteDocumentClassifier::SniffForHTML(data);
      } else if (canonical_mime_type_ == CROSS_SITE_DOCUMENT_MIME_TYPE_XML) {
        mime_type_confirmed = CrossSiteDocumentClassifier::SniffForXML(data);
      } else if (canonical_mime_type_ == CROSS_SITE_DOCUMENT_MIME_TYPE_JSON) {
        mime_type_confirmed = CrossSiteDocumentClassifier::SniffForJSON(data);
      } else if (canonical_mime_type_ == CROSS_SITE_DOCUMENT_MIME_TYPE_PLAIN) {
        // For responses labeled as plain text, only block them if the data
        // sniffs as one of the formats we would block in the first place.
        mime_type_confirmed = CrossSiteDocumentClassifier::SniffForHTML(data) ||
                              CrossSiteDocumentClassifier::SniffForXML(data) ||
                              CrossSiteDocumentClassifier::SniffForJSON(data);
      }
    }

    if (mime_type_confirmed) {
      // Block the response and throw away the data.
      const url::Origin& initiator = request()->initiator().value();
      DLOG(INFO) << "BLOCKING CROSS SITE DOCUMENT.  Initiator: " << initiator
                 << ", url: " << request()->url()
                 << ", type: " << resource_type_
                 << ", mime_type: " << canonical_mime_type_;

      TRACE_EVENT2("navigation",
                   "CrossSiteDocumentResourceHandler::ShouldBlockResponse",
                   "initiator", initiator.Serialize(), "url",
                   request()->url().possibly_invalid_spec());

      bytes_read = 0;
      blocked_read_completed_ = true;
    } else {
      // Allow the response through instead and proceed with reading more.
      // Copy sniffed data into the next handler's buffer before proceeding.
      DCHECK_LE(bytes_read, next_handler_buffer_size_);
      memcpy(next_handler_buffer_->data(), local_buffer_->data(), bytes_read);
      should_block_response_ = false;
    }

    // Clean up, whether we'll cancel or proceed from here.
    local_buffer_ = nullptr;
    next_handler_buffer_ = nullptr;
    next_handler_buffer_size_ = 0;
  }

  next_handler_->OnReadCompleted(bytes_read, std::move(controller));
}

void CrossSiteDocumentResourceHandler::OnResponseCompleted(
    const net::URLRequestStatus& status,
    std::unique_ptr<ResourceController> controller) {
  if (blocked_read_completed_) {
    // Report blocked responses as successful, rather than the cancellation
    // from OnWillRead.
    // TODO(creis): Preserve the original status.
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
  // backward compatible and we can enforce it always, regardless of Site
  // Isolation policy.
  auto* policy = ChildProcessSecurityPolicyImpl::GetInstance();
  if (!SiteIsolationPolicy::UseDedicatedProcessesForAllSites() &&
      !policy->IsIsolatedOrigin(url::Origin::Create(url))) {
    return false;
  }

  // Only block documents from HTTP(S) schemes.
  if (!CrossSiteDocumentClassifier::IsBlockableScheme(url))
    return false;

  DCHECK(request()->initiator().has_value());
  const url::Origin& initiator = request()->initiator().value();

  // Don't block same-site documents.
  if (CrossSiteDocumentClassifier::IsSameSite(initiator, url))
    return false;

  // Allow requests from file:// URLs for now.
  // TODO(creis): Limit this to when the allow_universal_access_from_file_urls
  // preference is set.  See https://crbug.com/789781.
  if (initiator.scheme() == url::kFileScheme)
    return false;

  // Only block if this is a request made from a renderer process.
  const ResourceRequestInfoImpl* info =
      ResourceRequestInfoImpl::ForRequest(request());
  if (!info || info->GetChildID() == -1)
    return false;

  // Look up MIME type.  If it doesn't claim to be a blockable type (i.e., HTML,
  // XML, JSON, or plain text), don't block it.
  std::string mime_type;
  request()->GetMimeType(&mime_type);
  canonical_mime_type_ =
      CrossSiteDocumentClassifier::GetCanonicalMimeType(mime_type);
  if (canonical_mime_type_ == CROSS_SITE_DOCUMENT_MIME_TYPE_OTHERS)
    return false;

  // Allow the response through if it has valid CORS headers.
  std::string cors_header;
  request()->GetResponseHeaderByName("access-control-allow-origin",
                                     &cors_header);
  if (CrossSiteDocumentClassifier::IsValidCorsHeaderSet(initiator, url,
                                                        cors_header)) {
    return false;
  }

  // TODO(creis): Don't block plugin requests with universal access. This could
  // be done by allowing resource_type_ == RESOURCE_TYPE_PLUGIN_RESOURCE unless
  // it had an Origin request header and IsValidCorsHeaderSet returned false.
  // (That would matter for plugins without universal access, which use CORS.)

  // Give embedder a chance to skip document blocking for this response.
  if (GetContentClient()->browser()->ShouldBypassDocumentBlocking(initiator,
                                                                  url)) {
    return false;
  }

  // We intend to block the response at this point.  However, we will plan to
  // sniff the contents to confirm the MIME type, to avoid blocking incorrectly
  // labeled JavaScript, JSONP, etc files.  Skip sniffing if there's a nosniff
  // header, though.
  std::string nosniff_header;
  request()->GetResponseHeaderByName("x-content-type-options", &nosniff_header);
  if (base::LowerCaseEqualsASCII(nosniff_header, "nosniff"))
    no_sniff_ = true;

  return true;
}

}  // namespace content
