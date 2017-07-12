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
    ResourceRequest& request,
    mojom::URLLoaderFactory& url_loader_factory)
    : cors_redirect_limit_(
          (request.fetch_request_mode == FETCH_REQUEST_MODE_CORS ||
           request.fetch_request_mode ==
               FETCH_REQUEST_MODE_CORS_WITH_FORCED_PREFLIGHT)
              ? kMaxCORSRedirects
              : 0),
      security_origin_(
          base::MakeUnique<url::Origin>(request.request_initiator.value())),
      cors_flag_(false),
      requestInfo_(requestInfo),
      request_(request),
      url_loader_factory_(url_loader_factory),
      client_(*this) {}

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
    const ResourceResponseHead& response,
    bool* defer) {
  // TODO(hintzed): Look up what DTL does with this flag
  //  suborigin_force_credentials_ = false;

  // TODO(hintzed): Make sure this checker is still called in DTL
  //   checker_.RedirectReceived();

  // TODO(hintzed): I don't think we need the following code here - but I am not
  // sure what the usecases are.
  /*
    if (request_.fetch_redirect_mode == FetchRedirectMode::MANUAL_MODE) {
      // We use |m_redirectMode| to check the original redirect mode. |request|
    is
      // a new request for redirect. So we don't set the redirect mode of it in
      // WebURLLoaderImpl::Context::OnReceivedRedirect().
      // TODO(hintzed): Not sure where to find this information in the request
    we
      // have;  DCHECK(request.UseStreamOnResponse());

      // There is no need to read the body of redirect response because there is
      // no way to read the body of opaque-redirect filtered response's internal
      // response.
      // TODO(horo): If we support any API which expose the internal body, we
    will
      // have to read the body. And also HTTPCache changes will be needed
      // because it doesn't store the body of redirect responses.
      ResponseReceived(resource, redirect_response,
                       WTF::MakeUnique<EmptyDataHandle>());

      if (client_) {
        DCHECK(actual_request_.IsNull());
        NotifyFinished(resource);
      }

      return false;
     }
    */

  // TODO(hintzed): Also does not feel like this belongs to CORS but to the DTL.
  // But how else would we make sure that the redirect is really canceled and
  // not followed without CORS checks?
  if (request_.fetch_redirect_mode == FetchRedirectMode::ERROR_MODE) {
    delegate_->CancelWithError(net::ERR_ABORTED);
  }

  // Allow same origin requests to continue after allowing clients to audit the
  // redirect.
  if (IsAllowedRedirect(redirect_info.new_url)) {
    // TODO (hintzed): Make sure DTL still keeps this code:
    //     client_->DidReceiveRedirectTo(request.Url());
    //     if (client_->IsDocumentThreadableLoaderClient()) {
    //       return static_cast<DocumentThreadableLoaderClient*>(client_)
    //           ->WillFollowRedirect(request, redirect_response);
    //     }
    return;
  }

  if (cors_redirect_limit_ <= 0) {
    // TODO(hintzed): How to pass error msg on?
    delegate_->CancelWithError(net::ERR_ABORTED);
    return;
  }

  --cors_redirect_limit_;

  /*  TODO(hintzed): how to propagate this information to DTL?
   if (GetDocument() && GetDocument()->GetFrame()) {
     probe::didReceiveCORSRedirectResponse(
         GetDocument()->GetFrame(), resource->Identifier(),
         GetDocument()->GetFrame()->Loader().GetDocumentLoader(),
         redirect_response, resource);
   }
   */

  CrossOriginAccessControl::RedirectStatus redirect_status =
      CrossOriginAccessControl::CheckRedirectLocation(redirect_info.new_url);
  if (redirect_status != CrossOriginAccessControl::kRedirectSuccess) {
    // TODO(hintzed): How to pass error msg on?
    delegate_->CancelWithError(net::ERR_ABORTED);
    return;
  }

  // The redirect response must pass the access control check if the CORS
  // flag is set.
  if (cors_flag_) {
    CrossOriginAccessControl::AccessStatus cors_status =
        CrossOriginAccessControl::CheckAccess(redirect_info.new_url, response,
                                              request_.fetch_credentials_mode,
                                              GetSecurityOrigin());

    if (cors_status != CrossOriginAccessControl::kAccessAllowed) {
      // TODO(hintzed): How to pass error msg on?
      delegate_->CancelWithError(net::ERR_ABORTED);
      return;
    }
  }

  /* TODO(hintzed): Do I have to consider this part here?
   client_->DidReceiveRedirectTo(request.Url());

   // FIXME: consider combining this with CORS redirect handling performed by
   // CrossOriginAccessControl::handleRedirect().
   ClearResource();
*/

  // If
  // - CORS flag is set, and
  // - the origin of the redirect target URL is not same origin with the origin
  //   of the current request's URL
  // set the source origin to a unique opaque origin.
  //
  // See https://fetch.spec.whatwg.org/#http-redirect-fetch.
  if (cors_flag_) {
    url::Origin original_origin(request_.url);
    url::Origin redirect_origin(redirect_info.new_url);

    if (!original_origin.IsSameOriginWith(redirect_origin))
      security_origin_ = base::MakeUnique<url::Origin>();
  }

  // Set |cors_flag_| so that further logic (corresponds to the main fetch in
  // the spec) will be performed with CORS flag set.
  // See https://fetch.spec.whatwg.org/#http-redirect-fetch.
  cors_flag_ = true;

  /* TODO(hintzed): Does not smell like it belongs to CORS - leave in DTL I
  suppose?
  // Save the referrer to use when following the redirect.
  override_referrer_ = true;
  referrer_after_redirect_ =
      Referrer(request.HttpReferrer(), request.GetReferrerPolicy());
*/

  /* TODO (hintzed): I don't think throttles are supposed to modify the request
  - I will look into this later, since if we are using a URLLoader instead of a
  Throttle, this will likely look different.

  ResourceRequest cross_origin_request(request);

  // Remove any headers that may have been added by the network layer
  that cause
  // access control to fail.
  cross_origin_request.ClearHTTPReferrer();
  cross_origin_request.ClearHTTPOrigin();
  cross_origin_request.ClearHTTPUserAgent();
  // Add any request headers which we previously saved from the
  // original request.
  for (const auto& header : request_headers_)
    cross_origin_request.SetHTTPHeaderField(header.key, header.value);
  MakeCrossOriginAccessRequest(cross_origin_request);
*/

  return;
}

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

void CORSURLLoaderThrottle::OnPreflightRedirect(
    const net::RedirectInfo& redirect_info,
    const ResourceResponseHead& response_head) {
  // TODO(hintzed): ReportResponseReceived(resource->Identifier(),
  // redirect_response);

  // TODO(hintzed): How to pass error msg on?
  // "Response for preflight is invalid (redirect)"
  delegate_->CancelWithError(net::ERR_ABORTED);
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

bool CORSURLLoaderThrottle::IsAllowedRedirect(const GURL& url) const {
  if (request_.fetch_request_mode ==
      FetchRequestMode::FETCH_REQUEST_MODE_NO_CORS)
    return true;

  // TODO(hintzed): In DTL, this check would potentially also check flags like
  // universal_access_ and SecurityPolicy::IsAccessWhiteListed. Do we need to do
  // the same here?
  return !cors_flag_ && GetSecurityOrigin().IsSameOriginWith(url::Origin(url));
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
