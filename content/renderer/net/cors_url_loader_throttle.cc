// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/net/cors_url_loader_throttle.h"
#include "base/memory/ptr_util.h"
#include "base/stl_util.h"
#include "content/renderer/net/cross_origin_access_control.h"
#include "content/renderer/net/cross_origin_preflight_result_cache.h"
#include "content/renderer/render_thread_impl.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "net/http/http_request_headers.h"
#include "third_party/WebKit/public/platform/modules/serviceworker/WebServiceWorkerResponseType.h"

#include "url/url_util.h"

namespace content {

namespace {

static bool IsMainThread() {
  return !!RenderThreadImpl::current();
}

}  // namespace

CORSURLLoaderThrottle::CORSURLLoaderThrottle(
    RequestInfo& requestInfo,
    ResourceRequest& url_request,
    mojom::URLLoaderFactory& url_loader_factory,
    scoped_refptr<base::SingleThreadTaskRunner> loading_task_runner)
    : cors_flag_(false),
      requestInfo_(requestInfo),
      request_(url_request),
      url_loader_factory_(url_loader_factory),
      client_(*this, loading_task_runner) {}

CORSURLLoaderThrottle::~CORSURLLoaderThrottle() {}

void CORSURLLoaderThrottle::WillStartRequest(
    const GURL& url,
    int load_flags,
    content::ResourceType resource_type,
    bool* defer) {
  // No CORS handling for navigation events:
  if (IsNavigation())
    return;

  // No CORS handling needed for same origin requests:
  if (IsSameOrigin())
    return;

  // Request is cross origin, but might not be allowed:
  if (!IsCORSAllowed()) {
    delegate_->CancelWithError(net::ERR_ABORTED);
  }

  cors_flag_ = true;

  if (NeedsPreflight()) {
    *defer = true;
    StartPreflightRequest();
  }
}

void CORSURLLoaderThrottle::WillRedirectRequest(
    const net::RedirectInfo& redirect_info,
    bool* defer) {}

void CORSURLLoaderThrottle::WillProcessResponse(
    const ResourceResponseHead& response,
    bool* defer) {
  if (response.was_fetched_via_service_worker) {
    // TODO(hintzed) perhaps keep doing this in DocumentThreadableLoader?:
    //    if (response.was_fetched_via_foreign_fetch())
    // loading_context_->RecordUseCount(WebFeature::kForeignFetchInterception);
    if (response.was_fallback_required_by_service_worker) {
      // At this point we must have m_fallbackRequestForServiceWorker. (For
      // SharedWorker the request won't be CORS or CORS-with-preflight,
      // therefore fallback-to-network is handled in the browser process when
      // the ServiceWorker does not call respondWith().)

      // TODO(hintzed): Not sure what this does or how to recreate:
      //        DCHECK(!fallback_request_for_service_worker_.IsNull());
      //        ReportResponseReceived(identifier, response);
      //        LoadFallbackRequestForServiceWorker();
      return;
    }

    // It's possible that we issue a fetch with request with non "no-cors"
    // mode but get an opaque filtered response if a service worker is involved.
    // We dispatch a CORS failure for the case.
    // TODO(yhirano): This is probably not spec conformant. Fix it after
    // https://github.com/w3c/preload/issues/100 is addressed.

    if (request_.fetch_request_mode != FETCH_REQUEST_MODE_NO_CORS &&
        response.response_type_via_service_worker ==
            blink::WebServiceWorkerResponseType::
                kWebServiceWorkerResponseTypeOpaque) {
      // TODO(hintzed): How to pass error msg on?
      delegate_->CancelWithError(net::ERR_ABORTED);
      return;
    }

    // TODO(hintzed): Do we still need this code here?
    //      fallback_request_for_service_worker_ = ResourceRequest();
    //      client_->DidReceiveResponse(identifier, response,
    //      std::move(handle));

    return;
  }

  // TODO(hintzed): This comment needs fixing once I understand the fallback
  // case:

  // Even if the request met the conditions to get handled by a Service Worker
  // in the constructor of this class (and therefore
  // |m_fallbackRequestForServiceWorker| is set), the Service Worker may skip
  // processing the request. Only if the request is same origin, the skipped
  // response may come here (wasFetchedViaServiceWorker() returns false) since
  // such a request doesn't have to go through the CORS algorithm by calling
  // loadFallbackRequestForServiceWorker().
  // FIXME: We should use |m_sameOriginRequest| when we will support Suborigins
  // (crbug.com/336894) for Service Worker.

  // TODO(hintzed): ??
  //  DCHECK(fallback_request_for_service_worker_.IsNull() ||
  //          GetSecurityOrigin()->CanRequest(
  //              fallback_request_for_service_worker_.Url()));
  //   fallback_request_for_service_worker_ = ResourceRequest();

  if ((request_.fetch_request_mode == FETCH_REQUEST_MODE_CORS ||
       request_.fetch_request_mode ==
           FETCH_REQUEST_MODE_CORS_WITH_FORCED_PREFLIGHT) &&
      cors_flag_) {
    CrossOriginAccessControl::AccessStatus cors_status =
        CrossOriginAccessControl::CheckAccess(request_.url, response,
                                              request_.fetch_credentials_mode,
                                              GetSecurityOrigin());

    if (cors_status != CrossOriginAccessControl::kAccessAllowed) {
      // TODO(hintzed): Do we keep this?
      // ReportResponseReceived(identifier, response);

      // TODO(hintzed): How to pass error msg on?
      delegate_->CancelWithError(net::ERR_ABORTED);
      return;
    }
  }
}

void CORSURLLoaderThrottle::OnPreflightResponse(
    scoped_refptr<ResourceResponse> response) {
  net::HttpRequestHeaders headers;

  CrossOriginAccessControl::AccessStatus access_status =
      CrossOriginAccessControl::CheckAccess(request_.url, response->head,
                                            request_.fetch_credentials_mode,
                                            GetSecurityOrigin());

  if (access_status != CrossOriginAccessControl::kAccessAllowed) {
    // TODO(hintzed): How to pass error msg on?
    delegate_->CancelWithError(net::ERR_ABORTED);
    return;
  }

  if (CrossOriginAccessControl::CheckPreflight(response) !=
      CrossOriginAccessControl::kPreflightSuccess) {
    // TODO(hintzed): How to pass error msg on?
    delegate_->CancelWithError(net::ERR_ABORTED);
    return;
  }

  if (request_.is_external_request_ &&
      CrossOriginAccessControl::CheckExternalPreflight(response) !=
          CrossOriginAccessControl::kPreflightSuccess) {
    // TODO(hintzed): How to pass error msg on?
    delegate_->CancelWithError(net::ERR_ABORTED);
    return;
  }

  std::unique_ptr<CrossOriginPreflightResultCacheItem> preflight_result =
      base::WrapUnique(new CrossOriginPreflightResultCacheItem(
          request_.fetch_credentials_mode));

  std::string access_control_error_description;
  net::HttpRequestHeaders request_headers;
  request_headers.AddHeadersFromString(request_.headers);

  if (!preflight_result->Parse(*response, access_control_error_description) ||
      !preflight_result->AllowsCrossOriginMethod(
          request_.method, access_control_error_description) ||
      !preflight_result->AllowsCrossOriginHeaders(
          request_headers, access_control_error_description)) {
    // TODO(hintzed): How to pass error msg on?
    delegate_->CancelWithError(net::ERR_ABORTED);
    return;
  }

  if (IsMainThread()) {
    // TODO(horo): Currently we don't support the CORS preflight cache on
    // worker thread when off-main-thread-fetch is enabled.
    // https:  crbug.com/443374
    CrossOriginPreflightResultCache::Shared().AppendEntry(
        GetSecurityOrigin().Serialize(), request_.url,
        std::move(preflight_result));
  }

  // All preflight checks passed, commence actuall request:
  delegate_->Resume();
}

bool CORSURLLoaderThrottle::IsCORSEnabled() {
  return request_.fetch_request_mode == FETCH_REQUEST_MODE_CORS ||
         request_.fetch_request_mode ==
             FETCH_REQUEST_MODE_CORS_WITH_FORCED_PREFLIGHT ||
         request_.is_external_request_;
}

bool CORSURLLoaderThrottle::IsSameOrigin() {
  return GetSecurityOrigin().IsSameOriginWith(url::Origin(request_.url));
}

bool CORSURLLoaderThrottle::IsCORSAllowed() {
  // FIXME(hintzed): Error message handling?

  // Cross-origin requests are only allowed certain registered schemes. We would
  // catch this when checking response headers later, but there is no reason to
  // send a request, preflighted or not, that's guaranteed to be denied.
  if (!base::ContainsValue(url::GetCORSEnabledSchemes(), request_.url.scheme()))
    return false;

  // Non-secure origins may not make "external requests":
  // https://wicg.github.io/cors-rfc1918/#integration-fetch
  if (!request_.initiated_in_secure_context && request_.is_external_request_)
    return false;

  return true;
}

bool CORSURLLoaderThrottle::IsNavigation() {
  return request_.fetch_request_mode == FETCH_REQUEST_MODE_NAVIGATE &&
         (request_.resource_type == RESOURCE_TYPE_MAIN_FRAME ||
          request_.resource_type == RESOURCE_TYPE_SUB_FRAME);
}

bool CORSURLLoaderThrottle::NeedsPreflight() {
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

  // Now, we need to check that the request passes the CORS preflight either by
  // issuing a CORS preflight or based on an entry in the CORS preflight cache.

  bool should_ignore_preflight_cache = false;

  if (!IsMainThread()) {
    // TODO(horo): Currently we don't support the CORS preflight cache on worker
    // thread when off-main-thread-fetch is enabled. See
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
          GetSecurityOrigin().Serialize(), request_.url,
          request_.fetch_credentials_mode, request_.method, request_.headers)) {
    return false;
  }

  return true;
}

void CORSURLLoaderThrottle::StartPreflightRequest() {
  ResourceRequest preflight_request =
      CrossOriginAccessControl::CreateAccessControlPreflightRequest(request_);

  std::vector<std::unique_ptr<URLLoaderThrottle>> throttles;

  loader_ = ThrottlingURLLoader::CreateLoaderAndStart(
      &url_loader_factory_, std::move(throttles), requestInfo_.routing_id,
      requestInfo_.request_id, requestInfo_.options, preflight_request,
      &client_,
      net::MutableNetworkTrafficAnnotationTag(NO_TRAFFIC_ANNOTATION_YET));
}

url::Origin CORSURLLoaderThrottle::GetSecurityOrigin() const {
  return request_.request_initiator.value();
}

}  // namespace content
