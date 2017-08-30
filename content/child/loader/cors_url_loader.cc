// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/child/loader/cors_url_loader.h"

#include "base/memory/ptr_util.h"
#include "base/stl_util.h"
#include "content/child/child_thread_impl.h"
#include "content/child/resource_dispatcher.h"
#include "content/public/common/console_message_level.h"
#include "content/public/common/origin_util.h"
#include "content/public/common/service_worker_modes.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "net/base/load_flags.h"
#include "net/http/http_request_headers.h"
#include "platform/network/HTTPHeaderMap.h"
#include "public/platform/WebCORS.h"
#include "public/platform/WebCORSPreflightResultCache.h"
#include "public/platform/WebSecurityOrigin.h"
#include "public/platform/WebString.h"
#include "third_party/WebKit/public/platform/modules/fetch/fetch_api_request.mojom.h"
#include "third_party/WebKit/public/web/WebSecurityPolicy.h"
#include "url/url_util.h"

namespace content {

namespace {

bool IsMainThread() {
  return !!ChildThreadImpl::current();
}

blink::WebCORS::AccessStatus CheckAccess(
    const GURL url,
    const FetchCredentialsMode credential_mode,
    const ResourceResponseHead& head,
    const url::Origin security_origin) {
  return blink::WebCORS::CheckAccess(
      url, head.headers->response_code(), head.headers.get(),
      static_cast<blink::WebURLRequest::FetchCredentialsMode>(credential_mode),
      security_origin);
}

std::string AccessControlErrorString(
    const blink::WebCORS::AccessStatus status,
    const ResourceResponseHead& head,
    const url::Origin security_origin,
    const RequestContextType fetch_request_context_type) {
  return blink::WebCORS::AccessControlErrorString(
             status, head.headers->response_code(), head.headers.get(),
             security_origin,
             static_cast<blink::WebURLRequest::RequestContext>(
                 fetch_request_context_type))
      .Ascii();
}

}  // namespace

CORSURLLoader::CORSURLLoader(
    int32_t routing_id,
    int32_t request_id,
    uint32_t options,
    const ResourceRequest& resource_request,
    mojom::URLLoaderClientPtr client,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation,
    mojom::URLLoaderFactory* network_loader_factory)
    : mode_(Mode::kUndefined),
      network_loader_factory_(network_loader_factory),
      forwarding_client_(std::move(client)),
      request_(resource_request),
      routing_id_(routing_id),
      request_id_(request_id),
      options_(options),
      traffic_annotation_(traffic_annotation),
      security_origin_(request_.request_initiator.has_value()
                           ? request_.request_initiator.value()
                           : url::Origin()) {
  DCHECK(network_loader_factory_);

  std::string error_description;

  // TODO(hintzed): I took a shortcut here compared to DTL - check if this is
  // ok.
  // No CORS handling for navigation events or same origin requests:
  if ((IsNavigation() || IsSameOriginOrWhitelisted(request_.url))) {
    cors_flag_ = false;
  } else if (request_.fetch_request_mode ==
             FetchRequestMode::FETCH_REQUEST_MODE_NO_CORS) {
    bool IsNoCORSAllowedContext = blink::WebCORS::IsNoCORSAllowedContext(
        static_cast<blink::WebURLRequest::RequestContext>(
            request_.fetch_request_context_type),
        static_cast<blink::WebURLRequest::ServiceWorkerMode>(
            request_.service_worker_mode));

    SECURITY_CHECK(IsNoCORSAllowedContext);
    cors_flag_ = false;
  } else {
    cors_flag_ = true;
  }

  // Request is cross origin, but might not be allowed:
  if (cors_flag_ && !IsCORSAllowed(error_description))
    DidFailAccessControlCheck(error_description);
  else if (cors_flag_ && NeedsPreflight())
    StartPreflightRequest();
  else
    StartActualRequest();
}

CORSURLLoader::~CORSURLLoader() {}

void CORSURLLoader::FollowRedirect() {
  network_loader_->FollowRedirect();
}

void CORSURLLoader::SetPriority(net::RequestPriority priority,
                                int32_t intra_priority_value) {
  network_loader_->SetPriority(priority, intra_priority_value);
}

// mojom::URLLoaderClient overrides:
void CORSURLLoader::OnReceiveResponse(
    const content::ResourceResponseHead& head,
    const base::Optional<net::SSLInfo>& ssl_info,
    mojom::DownloadedTempFilePtr downloaded_file) {
  if (mode_ == Mode::kPreflight)
    HandlePreflightResponse(head);
  else if (mode_ == Mode::kActualRequest)
    HandleActualResponse(head, ssl_info, std::move(downloaded_file));
  else
    NOTREACHED();
}

void CORSURLLoader::OnReceiveRedirect(
    const net::RedirectInfo& redirect_info,
    const content::ResourceResponseHead& head) {
  if (mode_ == Mode::kPreflight)
    HandlePreflightRedirect(redirect_info, head);
  else if (mode_ == Mode::kActualRequest)
    HandleActualRedirect(redirect_info, head);
  else
    NOTREACHED();
}

void CORSURLLoader::DidFailRedirectCheck() {
  DCHECK(mode_ == Mode::kUndefined || mode_ == Mode::kPreflight ||
         mode_ == Mode::kActualRequest);

  // Prevents any further callbacks into |forwarding_client_|.
  mode_ = Mode::kRedirectFailed;

  ResourceRequestCompletionStatus request_completion_status;
  request_completion_status.error_code = net::ERR_TOO_MANY_REDIRECTS;

  forwarding_client_->OnComplete(request_completion_status);
}

void CORSURLLoader::DidFailAccessControlCheck(std::string error_description) {
  DCHECK(mode_ == Mode::kUndefined || mode_ == Mode::kPreflight ||
         mode_ == Mode::kActualRequest);

  // Prevents any further callbacks into |forwarding_client_|.
  mode_ = Mode::kCORSFailed;

  ResourceRequestCompletionStatus request_completion_status;
  request_completion_status.error_code = net::ERR_ACCESS_DENIED;
  request_completion_status.error_description = error_description;

  forwarding_client_->OnComplete(request_completion_status);
}

bool CORSURLLoader::IsCORSEnabled() const {
  return request_.fetch_request_mode == FETCH_REQUEST_MODE_CORS ||
         request_.fetch_request_mode ==
             FETCH_REQUEST_MODE_CORS_WITH_FORCED_PREFLIGHT ||
         request_.cors.is_external_request;
}

bool CORSURLLoader::IsSameOriginOrWhitelisted(const GURL& url) const {
  return security_origin_.IsSameOriginWith(url::Origin(url)) ||
         IsOriginWhiteListedTrustworthy(url::Origin(url));
}

bool CORSURLLoader::IsAllowedRedirect(const GURL& url) const {
  if (request_.fetch_request_mode ==
      FetchRequestMode::FETCH_REQUEST_MODE_NO_CORS)
    return true;

  // TODO(hintzed): In DTL, this check would potentially also check flags like
  // universal_access_. Do we need to do the same here?
  return !cors_flag_ && IsSameOriginOrWhitelisted(url);
}

bool CORSURLLoader::IsCORSAllowed(std::string& error_description) const {
  // Cross-origin requests are only allowed certain registered schemes. We
  // would catch this when checking response headers later, but there is no
  // reason to send a request, preflighted or not, that's guaranteed to be
  // denied.
  if (!base::ContainsValue(url::GetCORSEnabledSchemes(),
                           request_.url.scheme())) {
    error_description =
        "Cross origin requests are only supported for protocol schemes: " +
        blink::WebCORS::ListOfCORSEnabledURLSchemes().Ascii() + ".";
    return false;
  }

  // Non-secure origins may not make "external requests":
  // https://wicg.github.io/cors-rfc1918/#integration-fetch
  if (!request_.initiated_in_secure_context &&
      request_.cors.is_external_request) {
    error_description =
        "Requests to internal network resources are not allowed from "
        "non-secure contexts (see https://goo.gl/Y0ZkNV). This is an "
        "experimental restriction which is part of "
        "'https://mikewest.github.io/cors-rfc1918/'.";
    return false;
  }

  if (request_.fetch_request_mode == FETCH_REQUEST_MODE_SAME_ORIGIN) {
    error_description = "Cross origin requests are not supported.";
    return false;
  }

  return true;
}

bool CORSURLLoader::IsNavigation() const {
  return request_.fetch_request_mode == FETCH_REQUEST_MODE_NAVIGATE &&
         (request_.resource_type == RESOURCE_TYPE_MAIN_FRAME ||
          request_.resource_type == RESOURCE_TYPE_SUB_FRAME);
}

bool CORSURLLoader::NeedsPreflight() const {
  net::HttpRequestHeaders request_headers;
  request_headers.AddHeadersFromString(request_.headers);

  // Enforce the CORS preflight for checking the Access-Control-Allow-External
  // header. The CORS preflight cache doesn't help for this purpose.
  if (request_.cors.is_external_request)
    return true;

  if (request_.cors.prevent_preflight)
    return false;

  if (request_.fetch_request_mode ==
      FETCH_REQUEST_MODE_CORS_WITH_FORCED_PREFLIGHT)
    return true;

  // We use ContainsOnlyCORSSafelistedOrForbiddenHeaders() here since
  // |request| may have been modified in the process of loading (not from
  // the user's input). For example, referrer. We need to accept them. For
  // security, we must reject forbidden headers/methods at the point we
  // accept user's input. Not here.
  if (blink::WebCORS::IsCORSSafelistedMethod(
          blink::WebString::FromASCII(request_.method)) &&
      blink::WebCORS::ContainsOnlyCORSSafelistedOrForbiddenHeaders(
          &request_headers)) {
    return false;
  }

  if (!IsMainThread()) {
    // TODO(horo): Currently we don't support the CORS preflight cache on
    // worker thread when off-main-thread-fetch is enabled. See
    // https://crbug.com/443374.
    return true;
  }

  return !blink::WebCORSPreflightResultCache::Shared().CanSkipPreflight(
      blink::WebString::FromASCII(security_origin_.Serialize()), request_.url,
      static_cast<blink::WebURLRequest::FetchCredentialsMode>(
          request_.fetch_credentials_mode),
      blink::WebString::FromASCII(request_.method), &request_headers);
}

void CORSURLLoader::StartPreflightRequest() {
  mode_ = Mode::kPreflight;
  ResourceRequest preflight = CreateAccessControlPreflightRequest(request_);
  DispatchRequest(preflight, ResourceDispatcher::MakeRequestID());
}

void CORSURLLoader::StartActualRequest() {
  mode_ = Mode::kActualRequest;

  if (cors_flag_)
    AddOriginHeader(request_);

  DispatchRequest(request_, request_id_);
}

void CORSURLLoader::AddOriginHeader(ResourceRequest& request) {
  net::HttpRequestHeaders request_headers;
  request_headers.AddHeadersFromString(request.headers);
  // TODO (hintzed): Origin headers are a mess at the moment because I would
  // want to handle them here, since this is where the CORS logic is, but here
  // I don't know enough about redirects, so this logic for now is in
  // DocumentThreadableLoader which then sets the Origin. Needs cleaning up
  // after we figured out how to handle redirects.
  // Also, FrameFetchContext does add Origin and Referrer, which feels kind of
  // messy.

  if (request_headers.HasHeader(net::HttpRequestHeaders::kOrigin))
    return;

  request_headers.SetHeader(net::HttpRequestHeaders::kOrigin,
                            security_origin_.Serialize());
  if (security_origin_.suborigin() != "") {
    request_headers.SetHeader(net::HttpRequestHeaders::kSuborigin,
                              security_origin_.suborigin());
  }

  request.headers = request_headers.ToString();
}

void CORSURLLoader::DispatchRequest(const ResourceRequest& request,
                                    const int32_t request_id) {
  auto url_loader_request = mojo::MakeRequest(&network_loader_);

  mojom::URLLoaderClientPtr client_ptr;

  network_client_binding_.reset(
      new mojo::Binding<mojom::URLLoaderClient>(this));
  network_client_binding_->Bind(mojo::MakeRequest(&client_ptr));

  network_loader_factory_->CreateLoaderAndStart(
      std::move(url_loader_request), routing_id_, request_id, options_, request,
      std::move(client_ptr),
      net::MutableNetworkTrafficAnnotationTag(traffic_annotation_));
}

void CORSURLLoader::HandlePreflightResponse(const ResourceResponseHead& head) {
  DCHECK_EQ(mode_, Mode::kPreflight);

  net::HttpRequestHeaders request_headers;
  request_headers.AddHeadersFromString(request_.headers);
  blink::WebHTTPHeaderMap request_header_map(&request_headers);

  blink::WebHTTPHeaderMap response_headers(head.headers.get());

  blink::WebCORS::AccessStatus access_status = CheckAccess(
      request_.url, request_.fetch_credentials_mode, head, security_origin_);

  if (access_status != blink::WebCORS::kAccessAllowed) {
    DidFailAccessControlCheck(
        "Response to preflight request doesn't pass access control check: " +
        AccessControlErrorString(access_status, head, security_origin_,
                                 request_.fetch_request_context_type));
    return;
  }

  blink::WebCORS::PreflightStatus preflight_status =
      blink::WebCORS::CheckPreflight(head.headers->response_code());
  if (preflight_status != blink::WebCORS::kPreflightSuccess) {
    DidFailAccessControlCheck(
        blink::WebCORS::PreflightErrorString(preflight_status, response_headers,
                                             head.headers->response_code())
            .Ascii());
    return;
  }

  if (request_.cors.is_external_request) {
    blink::WebCORS::PreflightStatus external_preflight_status =
        blink::WebCORS::CheckExternalPreflight(response_headers);
    if (external_preflight_status != blink::WebCORS::kPreflightSuccess) {
      DidFailAccessControlCheck(blink::WebCORS::PreflightErrorString(
                                    external_preflight_status, response_headers,
                                    head.headers->response_code())
                                    .Ascii());
      return;
    }
  }

  blink::WebString error_description;
  std::unique_ptr<blink::WebCORSPreflightResultCacheItem> result =
      blink::WebCORSPreflightResultCacheItem::Create(
          static_cast<blink::WebURLRequest::FetchCredentialsMode>(
              request_.fetch_credentials_mode),
          response_headers, error_description);

  if (!result ||
      !result->AllowsCrossOriginMethod(
          blink::WebString::FromASCII(request_.method), error_description) ||
      !result->AllowsCrossOriginHeaders(request_header_map,
                                        error_description)) {
    DidFailAccessControlCheck(error_description.Ascii());
    return;
  }

  if (IsMainThread()) {
    // TODO(horo): Currently we don't support the CORS preflight cache on
    // worker thread when off-main-thread-fetch is enabled.
    // https:  crbug.com/443374
    blink::WebCORSPreflightResultCache::Shared().AppendEntry(
        blink::WebString::FromASCII(security_origin_.Serialize()), request_.url,
        std::move(result));
  }

  // All preflight checks passed, commence actual request:
  StartActualRequest();
}

void CORSURLLoader::HandlePreflightRedirect(
    const net::RedirectInfo& redirect_info,
    const ResourceResponseHead& response_head) {
  DCHECK_EQ(mode_, Mode::kPreflight);

  DidFailAccessControlCheck("Response for preflight is invalid (redirect)");
}

void CORSURLLoader::HandleActualResponse(
    const ResourceResponseHead& head,
    const base::Optional<net::SSLInfo>& ssl_info,
    mojom::DownloadedTempFilePtr downloaded_file) {
  DCHECK_EQ(mode_, Mode::kActualRequest);

  if (head.was_fetched_via_service_worker) {
    // TODO(hintzed): Again, I think these parts would remain in DTL?
    //    if (head.was_fetched_via_foreign_fetch) {
    //      loading_context_->GetFetchContext()->CountUsage(
    //          WebFeature::kForeignFetchInterception);
    //    }
    //    if (head.was_fallback_required_by_service_worker) {
    //      // At this point we must have m_fallbackRequestForServiceWorker.
    //      (For
    //      // SharedWorker the request won't be CORS or CORS-with-preflight,
    //      // therefore fallback-to-network is handled in the browser process
    //      when
    //      // the ServiceWorker does not call respondWith().)
    //      DCHECK(!fallback_request_for_service_worker_.IsNull());
    //      ReportResponseReceived(identifier, response);
    //      LoadFallbackRequestForServiceWorker();
    //      return;
    //    }

    // It's possible that we issue a fetch with request with non "no-cors"
    // mode but get an opaque filtered response if a service worker is
    // involved. We dispatch a CORS failure for the case.
    // TODO(yhirano): This is probably not spec conformant. Fix it after
    // https://github.com/w3c/preload/issues/100 is addressed.

    // TODO(hintzed): This keeps the following check from breaking a number of
    // tests in http/tests/fetch/serviceworker-proxied/thorough/nocors.html
    // The breakage seems to be related to yhirano's comment as it is indeed a
    // service worker case with no-cors and an opaque response. Not sure what to
    // do, though, since the request would not have gone through
    // DocumentThreadableLoader before.

    // TODO(hintzed): Having this logic breaks other tests as well, e.g.
    // external/wpt/service-workers/service-worker/fetch-event-redirect.https.html.
    // Lets see if not having that check also breaks things?
    //    bool temporary_skip =
    //        request_.fetch_request_context_type ==
    //        REQUEST_CONTEXT_TYPE_SCRIPT;
    //
    //    if (!temporary_skip &&
    //        request_.fetch_request_mode != FETCH_REQUEST_MODE_CORS &&
    //        head.response_type_via_service_worker ==
    //            network::mojom::FetchResponseType::kOpaque) {
    //      DidFailAccessControlCheck(AccessControlErrorString(
    //          blink::WebCORS::kInvalidResponse, head, security_origin_,
    //          request_.fetch_request_context_type));
    //      return;
    //    }

    forwarding_client_->OnReceiveResponse(head, ssl_info,
                                          std::move(downloaded_file));
    return;
  }

  // Even if the request met the conditions to get handled by a Service Worker
  // in the constructor of this class (and therefore
  // |m_fallbackRequestForServiceWorker| is set), the Service Worker may skip
  // processing the request. Only if the request is same origin, the skipped
  // response may come here (wasFetchedViaServiceWorker() returns false) since
  // such a request doesn't have to go through the CORS algorithm by calling
  // loadFallbackRequestForServiceWorker().
  // FIXME: We should use |m_sameOriginRequest| when we will support Suborigins
  // (crbug.com/336894) for Service Worker.

  // TODO(hintzed): I think this part (and the comment above) should remain in
  // DTL?
  //   DCHECK(fallback_request_for_service_worker_.IsNull() ||
  //          GetSecurityOrigin()->CanRequest(
  //              fallback_request_for_service_worker_.Url()));
  //   fallback_request_for_service_worker_ = ResourceRequest();

  if (IsCORSEnabled() && cors_flag_) {
    // TODO(hintzed): I think the request_.url ist the wrong one to use in face
    // of redirects but atm don't know from where to get the 'correct' url.
    blink::WebCORS::AccessStatus cors_status = CheckAccess(
        request_.url, request_.fetch_credentials_mode, head, security_origin_);

    if (cors_status != blink::WebCORS::AccessStatus ::kAccessAllowed) {
      DidFailAccessControlCheck(
          AccessControlErrorString(cors_status, head, security_origin_,
                                   request_.fetch_request_context_type));
      return;
    }
  }

  forwarding_client_->OnReceiveResponse(head, ssl_info,
                                        std::move(downloaded_file));
}

void CORSURLLoader::HandleActualRedirect(const net::RedirectInfo& redirect_info,
                                         const ResourceResponseHead& head) {
  DCHECK_EQ(mode_, Mode::kActualRequest);

  // TODO(hintzed):
  //    suborigin_force_credentials_ = false;

  // TODO(hintzed): From what I understand from DocumentThreadableLoader,
  // we would not perform CORS checks in manual mode because the result will
  // be opaque anyways.
  if (request_.fetch_redirect_mode == FetchRedirectMode::MANUAL_MODE) {
    forwarding_client_->OnReceiveRedirect(redirect_info, head);
    return;
  }

  // TODO(hintzed) Again, something I feel DocumentThreadableLoader would need
  // to handle this since it is not related to CORS. But I think we would want
  // to make sure that the redirect is not followed, e.g. by setting a certain
  // mode or so.
  if (request_.fetch_redirect_mode == FetchRedirectMode::ERROR_MODE) {
    forwarding_client_->OnReceiveRedirect(redirect_info, head);
    return;
  }

  // Allow same origin requests to continue
  if (IsAllowedRedirect(redirect_info.new_url)) {
    forwarding_client_->OnReceiveRedirect(redirect_info, head);
    return;
  }

  blink::WebCORS::RedirectStatus redirect_status =
      blink::WebCORS::CheckRedirectLocation(redirect_info.new_url);

  // TODO(hintzed): Do I use the original url here or the one from the current
  // request, in case it is a chain of redirects. If so, can I get the url
  // without tracking it myself?
  if (redirect_status != blink::WebCORS::RedirectStatus::kRedirectSuccess) {
    DidFailAccessControlCheck("Redirect from '" + request_.url.spec() +
                              "' has been blocked by CORS policy: " +
                              blink::WebCORS::RedirectErrorString(
                                  redirect_status, redirect_info.new_url)
                                  .Ascii());

    return;
  } else if (cors_flag_) {
    // The redirect response must pass the access control check if the CORS
    // flag is set.
    blink::WebCORS::AccessStatus cors_status =
        CheckAccess(redirect_info.new_url, request_.fetch_credentials_mode,
                    head, security_origin_);
    if (cors_status != blink::WebCORS::kAccessAllowed) {
      // TODO(hintzed): Do I use the original url here or the one from the
      // current request, in case it is a chain of redirects. If so, can I get
      // the url without tracking it myself?
      DidFailAccessControlCheck(
          "Redirect from '" + request_.url.spec() + "' to '" +
          redirect_info.new_url.spec() + "' has been blocked by CORS policy: " +
          AccessControlErrorString(cors_status, head, security_origin_,
                                   request_.fetch_request_context_type));
      return;
    }
  }

  forwarding_client_->OnReceiveRedirect(redirect_info, head);

  // TODO(hintzed): Communicate unique origin back to DTL?
  //  // If
  //  // - CORS flag is set, and
  //  // - the origin of the redirect target URL is not same origin with the
  //  origin
  //  //   of the current request's URL
  //  // set the source origin to a unique opaque origin.
  //  //
  //  // See https://fetch.spec.whatwg.org/#http-redirect-fetch.
  //
  //  // TODO(hintzed): The comment above indicates I need the _current_
  //  request's
  //  // origin, so I should figure out how to obtain that.
  //  if (cors_flag_ &&
  //     !security_origin_.IsSameOriginWith(url::Origin(redirect_info.new_url)))
  //    security_origin_ = url::Origin();
  //
  //  forwarding_client_->OnReceiveRedirect(redirect_info, head);
}

ResourceRequest CORSURLLoader::CreateAccessControlPreflightRequest(
    const ResourceRequest& request) {
  const GURL& request_url = request.url;

  DCHECK(request_url.username().empty());
  DCHECK(request_url.password().empty());

  ResourceRequest preflight_request;
  preflight_request.method = "OPTIONS";
  preflight_request.url = request.url;
  preflight_request.priority = request.priority;
  preflight_request.request_context = request.request_context;
  preflight_request.fetch_credentials_mode = FETCH_CREDENTIALS_MODE_OMIT;
  preflight_request.service_worker_mode = ServiceWorkerMode::NONE;
  preflight_request.resource_type = request.resource_type;
  preflight_request.referrer = request_.referrer;
  preflight_request.referrer_policy = request_.referrer_policy;

  preflight_request.load_flags |= net::LOAD_DO_NOT_SAVE_COOKIES;
  preflight_request.load_flags |= net::LOAD_DO_NOT_SEND_COOKIES;
  preflight_request.load_flags |= net::LOAD_DO_NOT_SEND_AUTH_DATA;

  net::HttpRequestHeaders preflight_headers;
  preflight_headers.SetHeader("Access-Control-Request-Method", request.method);

  if (request.cors.is_external_request)
    preflight_headers.SetHeader("Access-Control-Request-External", "true");

  if (!request.headers.empty()) {
    net::HttpRequestHeaders request_headers;
    request_headers.AddHeadersFromString(request.headers);

    blink::WebHTTPHeaderMap headers(&request_headers);

    std::string access_control_request_headers =
        blink::WebCORS::CreateAccessControlRequestHeadersHeader(headers)
            .Latin1();

    if (!access_control_request_headers.empty()) {
      preflight_headers.SetHeader("Access-Control-Request-Headers",
                                  access_control_request_headers);
    }
  }

  preflight_request.headers = preflight_headers.ToString();

  // TODO(hintzed): probably wrong, given how redirect currently work?
  preflight_request.request_initiator = security_origin_;

  AddOriginHeader(preflight_request);

  return preflight_request;
}

void CORSURLLoader::OnDataDownloaded(int64_t data_length,
                                     int64_t encoded_length) {
  if (mode_ == Mode::kActualRequest)
    forwarding_client_->OnDataDownloaded(data_length, encoded_length);
}

void CORSURLLoader::OnUploadProgress(int64_t current_position,
                                     int64_t total_size,
                                     base::OnceCallback<void()> ack_callback) {
  if (mode_ == Mode::kActualRequest)
    forwarding_client_->OnUploadProgress(current_position, total_size,
                                         std::move(ack_callback));
}

void CORSURLLoader::OnReceiveCachedMetadata(const std::vector<uint8_t>& data) {
  if (mode_ == Mode::kActualRequest)
    forwarding_client_->OnReceiveCachedMetadata(data);
}

void CORSURLLoader::OnTransferSizeUpdated(int32_t transfer_size_diff) {
  if (mode_ == Mode::kActualRequest)
    forwarding_client_->OnTransferSizeUpdated(transfer_size_diff);
}

void CORSURLLoader::OnStartLoadingResponseBody(
    mojo::ScopedDataPipeConsumerHandle body) {
  if (mode_ == Mode::kActualRequest)
    forwarding_client_->OnStartLoadingResponseBody(std::move(body));
}

void CORSURLLoader::OnComplete(
    const content::ResourceRequestCompletionStatus& completion_status) {
  if (mode_ == Mode::kActualRequest)
    forwarding_client_->OnComplete(completion_status);
}

}  // namespace content
