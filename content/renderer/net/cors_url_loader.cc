// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/net/cors_url_loader.h"
#include "base/memory/ptr_util.h"
#include "base/stl_util.h"
#include "content/renderer/net/cross_origin_access_control.h"
#include "content/renderer/net/cross_origin_preflight_result_cache.h"
#include "content/renderer/render_thread_impl.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/strong_associated_binding.h"
#include "net/http/http_request_headers.h"
#include "third_party/WebKit/public/platform/modules/serviceworker/WebServiceWorkerResponseType.h"
#include "url/url_util.h"

namespace content {

namespace {

static bool IsMainThread() {
  return !!RenderThreadImpl::current();
}

}  // namespace

// static
void CORSURLLoader::CreateAndStart(
    mojom::URLLoaderFactory* factory,
    mojom::URLLoaderAssociatedRequest request,
    int32_t routing_id,
    int32_t request_id,
    uint32_t options,
    const ResourceRequest& resource_request,
    mojom::URLLoaderClientPtr client,
    const net::NetworkTrafficAnnotationTag& traffic_annotation) {
  mojo::MakeStrongAssociatedBinding(
      base::MakeUnique<CORSURLLoader>(factory, routing_id, request_id, options,
                                      resource_request, std::move(client),
                                      traffic_annotation),
      std::move(request));
}

CORSURLLoader::CORSURLLoader(
    mojom::URLLoaderFactory* factory,
    int32_t routing_id,
    int32_t request_id,
    uint32_t options,
    const ResourceRequest& resource_request,
    mojom::URLLoaderClientPtr client,
    const net::NetworkTrafficAnnotationTag& traffic_annotation)
    : mode_(Mode::ActualRequest),
      factory_(factory),
      default_network_client_binding_(this),
      forwarding_client_(std::move(client)),
      request_(resource_request),
      routing_id_(routing_id),
      request_id_(request_id),
      options_(options),
      traffic_annotation_(traffic_annotation) {
  // Checks if the CORS-preflight fetch is needed, and if so performs the
  // preflight first.
  // Bind this as the client of default, underlying network, and
  // call the given factory's CreateLoaderAndStart to start the original
  // request or preflight request.

  // No CORS handling for navigation events or same origin requests:
  cors_flag_ = !(IsNavigation() || IsSameOrigin());

  // Request is cross origin, but might not be allowed:
  if (cors_flag_ && !IsCORSAllowed())
    DidFailAccessControlCheck();
  else if (NeedsPreflight())
    StartPreflightRequest();
  else
    StartActualRequest();
}

CORSURLLoader::~CORSURLLoader() {}

void CORSURLLoader::FollowRedirect() {}

void CORSURLLoader::SetPriority(net::RequestPriority priority,
                                int intra_priority_value) {}

// mojom::URLLoaderClient overrides:
void CORSURLLoader::OnReceiveResponse(
    const content::ResourceResponseHead& head,
    const base::Optional<net::SSLInfo>& ssl_info,
    mojom::DownloadedTempFilePtr downloaded_file) {
  if (mode_ == Mode::Preflight)
    HandlePreflightResponse(head);
  else if (mode_ == Mode::ActualRequest)
    HandleActualResponse(head, ssl_info, std::move(downloaded_file));
  else
    NOTREACHED();
}

void CORSURLLoader::OnReceiveRedirect(
    const net::RedirectInfo& redirect_info,
    const content::ResourceResponseHead& head) {
  if (mode_ == Mode::Preflight)
    HandlePreflightRedirect(redirect_info, head);
  else if (mode_ == Mode::ActualRequest)
    HandleActualRedirect(redirect_info, head);
  else
    NOTREACHED();
}

void CORSURLLoader::DidFailAccessControlCheck() {
  DCHECK(mode_ == Mode::Preflight || mode_ == Mode::ActualRequest);

  mode_ = Mode::CORSFailed;

  ResourceRequestCompletionStatus request_completion_status;
  // TODO(hintzed): Not sure which error code to use?
  request_completion_status.error_code = net::ERR_CONNECTION_RESET;

  // TODO(hintzed): Spec says to clear the cache at this point (I guess?)

  forwarding_client_->OnComplete(request_completion_status);
}

bool CORSURLLoader::IsCORSEnabled() {
  return request_.fetch_request_mode == FETCH_REQUEST_MODE_CORS ||
         request_.fetch_request_mode ==
             FETCH_REQUEST_MODE_CORS_WITH_FORCED_PREFLIGHT ||
         request_.is_external_request_;
}

bool CORSURLLoader::IsSameOrigin() {
  return GetSecurityOrigin().IsSameOriginWith(url::Origin(request_.url));
}

bool CORSURLLoader::IsAllowedRedirect(const GURL& url) const {
  if (request_.fetch_request_mode ==
      FetchRequestMode::FETCH_REQUEST_MODE_NO_CORS)
    return true;

  // TODO(hintzed): In DTL, this check would potentially also check flags like
  // universal_access_ and SecurityPolicy::IsAccessWhiteListed. Do we need to
  // do the same here?
  return !cors_flag_ && GetSecurityOrigin().IsSameOriginWith(url::Origin(url));
}

bool CORSURLLoader::IsCORSAllowed() {
  // FIXME(hintzed): Error message handling?

  // Cross-origin requests are only allowed certain registered schemes. We
  // would catch this when checking response headers later, but there is no
  // reason to send a request, preflighted or not, that's guaranteed to be
  // denied.
  if (!base::ContainsValue(url::GetCORSEnabledSchemes(), request_.url.scheme()))
    return false;

  // Non-secure origins may not make "external requests":
  // https://wicg.github.io/cors-rfc1918/#integration-fetch
  if (!request_.initiated_in_secure_context && request_.is_external_request_)
    return false;

  return true;
}

bool CORSURLLoader::IsNavigation() {
  return request_.fetch_request_mode == FETCH_REQUEST_MODE_NAVIGATE &&
         (request_.resource_type == RESOURCE_TYPE_MAIN_FRAME ||
          request_.resource_type == RESOURCE_TYPE_SUB_FRAME);
}

bool CORSURLLoader::NeedsPreflight() {
  net::HttpRequestHeaders request_headers;
  request_headers.AddHeadersFromString(request_.headers);

  // Enforce the CORS preflight for checking the Access-Control-Allow-External
  // header. The CORS preflight cache doesn't help for this purpose.
  if (request_.is_external_request_)
    return true;

  if (request_.prevent_preflight)
    return false;

  if (request_.fetch_request_mode ==
      FETCH_REQUEST_MODE_CORS_WITH_FORCED_PREFLIGHT)
    return true;

  // We use ContainsOnlyCORSSafelistedOrForbiddenHeaders() here since
  // |request| may have been modified in the process of loading (not from
  // the user's input). For example, referrer. We need to accept them. For
  // security, we must reject forbidden headers/methods at the point we
  // accept user's input. Not here.
  if (CrossOriginAccessControl::IsCORSSafelistedMethod(request_.method) &&
      CrossOriginAccessControl::ContainsOnlyCORSSafelistedOrForbiddenHeaders(
          request_headers)) {
    return false;
  }

  // Now, we need to check that the request passes the CORS preflight either
  // by issuing a CORS preflight or based on an entry in the CORS preflight
  // cache.

  bool should_ignore_preflight_cache = false;

  if (!IsMainThread()) {
    // TODO(horo): Currently we don't support the CORS preflight cache on
    // worker thread when off-main-thread-fetch is enabled. See
    // https://crbug.com/443374.
    should_ignore_preflight_cache = true;
  } else {
    // Prevent use of the CORS preflight cache when instructed by the DevTools
    // not to use caches.

    // TODO(hintzed):
    //    probe::shouldForceCORSPreflight(GetDocument(),
    //                                    &should_ignore_preflight_cache);
  }

  if (should_ignore_preflight_cache ||
      !CrossOriginPreflightResultCache::Shared().CanSkipPreflight(
          GetSecurityOrigin(), request_.url, request_.fetch_credentials_mode,
          request_.method, request_.headers)) {
    return false;
  }

  return true;
}

void CORSURLLoader::StartPreflightRequest() {
  mode_ = Mode::Preflight;
  ResourceRequest preflight_request =
      CrossOriginAccessControl::CreateAccessControlPreflightRequest(request_);
  // TODO(hintzed): Start preflight request;
}

void CORSURLLoader::StartActualRequest() {
  mode_ = Mode::ActualRequest;
  DispatchRequest(request_);
}

void CORSURLLoader::DispatchRequest(const ResourceRequest& request) {
  auto url_loader_request = mojo::MakeRequest(&default_network_loader_);

  mojom::URLLoaderClientPtr client_ptr;
  default_network_client_binding_.Bind(mojo::MakeRequest(&client_ptr));

  factory_->CreateLoaderAndStart(
      std::move(url_loader_request), routing_id_, request_id_, options_,
      request_, std::move(client_ptr),
      net::MutableNetworkTrafficAnnotationTag(traffic_annotation_));
}

void CORSURLLoader::HandlePreflightResponse(const ResourceResponseHead& head) {
  DCHECK_EQ(mode_, Mode::Preflight);

  net::HttpRequestHeaders headers;

  CrossOriginAccessControl::AccessStatus access_status =
      CrossOriginAccessControl::CheckAccess(request_.url, head,
                                            request_.fetch_credentials_mode,
                                            GetSecurityOrigin());

  if (access_status != CrossOriginAccessControl::kAccessAllowed) {
    // TODO(hintzed): How to pass error msg on?
    // TODO(hintzed):delegate_->CancelWithError(net::ERR_ABORTED);
    return;
  }

  if (CrossOriginAccessControl::CheckPreflight(head) !=
      CrossOriginAccessControl::kPreflightSuccess) {
    // TODO(hintzed): How to pass error msg on?
    // TODO(hintzed): delegate_->CancelWithError(net::ERR_ABORTED);
    return;
  }

  if (request_.is_external_request_ &&
      CrossOriginAccessControl::CheckExternalPreflight(head) !=
          CrossOriginAccessControl::kPreflightSuccess) {
    // TODO(hintzed): How to pass error msg on?
    // TODO(hintzed):delegate_->CancelWithError(net::ERR_ABORTED);
    return;
  }

  std::unique_ptr<CrossOriginPreflightResultCacheItem> preflight_result =
      base::WrapUnique(new CrossOriginPreflightResultCacheItem(
          request_.fetch_credentials_mode));

  std::string access_control_error_description;
  net::HttpRequestHeaders request_headers;
  request_headers.AddHeadersFromString(request_.headers);

  if (!preflight_result->Parse(head, access_control_error_description) ||
      !preflight_result->AllowsCrossOriginMethod(
          request_.method, access_control_error_description) ||
      !preflight_result->AllowsCrossOriginHeaders(
          request_headers, access_control_error_description)) {
    // TODO(hintzed): How to pass error msg on?
    // TODO(hintzed): delegate_->CancelWithError(net::ERR_ABORTED);
    return;
  }

  if (IsMainThread()) {
    // TODO(horo): Currently we don't support the CORS preflight cache on
    // worker thread when off-main-thread-fetch is enabled.
    // https:  crbug.com/443374
    CrossOriginPreflightResultCache::Shared().AppendEntry(
        GetSecurityOrigin(), request_.url, std::move(preflight_result));
  }

  // All preflight checks passed, commence actual request:
  StartActualRequest();
}

void CORSURLLoader::HandlePreflightRedirect(
    const net::RedirectInfo& redirect_info,
    const ResourceResponseHead& response_head) {
  DCHECK_EQ(mode_, Mode::Preflight);

  // TODO(hintzed): ReportResponseReceived(resource->Identifier(),
  // redirect_response);

  // TODO(hintzed): How to pass error msg on?
  // "Response for preflight is invalid (redirect)"
  DidFailAccessControlCheck();
}

void CORSURLLoader::HandleActualResponse(
    const ResourceResponseHead& head,
    const base::Optional<net::SSLInfo>& ssl_info,
    mojom::DownloadedTempFilePtr downloaded_file) {
  DCHECK_EQ(mode_, Mode::ActualRequest);

  // TODO(hintzed): Perform CORS check and tainting.

  forwarding_client_->OnReceiveResponse(head, ssl_info,
                                        std::move(downloaded_file));
}

void CORSURLLoader::HandleActualRedirect(const net::RedirectInfo& redirect_info,
                                         const ResourceResponseHead& head) {
  DCHECK_EQ(mode_, Mode::ActualRequest);

  // TODO(hintzed): Perform CORS check and adjust the request and proceed,
  // or just terminate the request (i.e. calling forwarding client's
  // OnComplete with error code).

  forwarding_client_->OnReceiveRedirect(redirect_info, head);
}

url::Origin CORSURLLoader::GetSecurityOrigin() const {
  return request_.request_initiator.value();
}

void CORSURLLoader::OnDataDownloaded(int64_t data_length,
                                     int64_t encoded_length) {
  if (mode_ == Mode::ActualRequest)
    forwarding_client_->OnDataDownloaded(data_length, encoded_length);
}

void CORSURLLoader::OnUploadProgress(int64_t current_position,
                                     int64_t total_size,
                                     base::OnceCallback<void()> ack_callback) {
  if (mode_ == Mode::ActualRequest)
    forwarding_client_->OnUploadProgress(current_position, total_size,
                                         std::move(ack_callback));
}

void CORSURLLoader::OnReceiveCachedMetadata(const std::vector<uint8_t>& data) {
  if (mode_ == Mode::ActualRequest)
    forwarding_client_->OnReceiveCachedMetadata(data);
}

void CORSURLLoader::OnTransferSizeUpdated(int32_t transfer_size_diff) {
  if (mode_ == Mode::ActualRequest)
    forwarding_client_->OnTransferSizeUpdated(transfer_size_diff);
}

void CORSURLLoader::OnStartLoadingResponseBody(
    mojo::ScopedDataPipeConsumerHandle body) {
  if (mode_ == Mode::ActualRequest)
    forwarding_client_->OnStartLoadingResponseBody(std::move(body));
}

void CORSURLLoader::OnComplete(
    const content::ResourceRequestCompletionStatus& completion_status) {
  if (mode_ == Mode::ActualRequest)
    forwarding_client_->OnComplete(completion_status);
}

}  // namespace content
