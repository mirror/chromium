// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/net/cors_url_loader_throttle.h"
#include "base/stl_util.h"
#include "content/renderer/net/cross_origin_access_control.h"
#include "content/renderer/net/cross_origin_preflight_result_cache.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "net/http/http_request_headers.h"

#include "url/url_util.h"

namespace content {

CORSURLLoaderThrottle::CORSURLLoaderThrottle(
    RequestInfo& requestInfo,
    ResourceRequest& url_request,
    mojom::URLLoaderFactory& url_loader_factory,
    scoped_refptr<base::SingleThreadTaskRunner> loading_task_runner)
    : requestInfo_(requestInfo),
      request_(url_request),
      url_loader_factory_(url_loader_factory),
      client_(*this, requestInfo.request_id, loading_task_runner) {}

CORSURLLoaderThrottle::~CORSURLLoaderThrottle() {}

void CORSURLLoaderThrottle::WillStartRequest(
    const GURL& url,
    int load_flags,
    content::ResourceType resource_type,
    bool* defer) {
  DCHECK_EQ(0u, pending_preflights_);

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

  if (NeedsPreflight()) {
    *defer = true;
    StartPreflightRequest();
  }
}

void CORSURLLoaderThrottle::WillRedirectRequest(
    const net::RedirectInfo& redirect_info,
    bool* defer) {}

void CORSURLLoaderThrottle::WillProcessResponse(bool* defer) {}

void CORSURLLoaderThrottle::OnPreflightResponse(
    scoped_refptr<ResourceResponse> response) {
  CrossOriginAccessControl::AccessStatus access_status =
      CrossOriginAccessControl::CheckAccess(request_.url, response,
                                            request_.fetch_credentials_mode,
                                            request_.request_initiator.value());

  if (access_status)
    ;

  CrossOriginAccessControl::PreflightStatus preflight_status;
  if (request_.is_external_request_) {
    preflight_status =
        CrossOriginAccessControl::CheckExternalPreflight(response);
  } else {
    preflight_status = CrossOriginAccessControl::CheckPreflight(response);
  }

  /*

    CrossOriginAccessControl::AccessStatus cors_status =
        CrossOriginAccessControl::CheckAccess(
            response, actual_request_.GetFetchCredentialsMode(),
            GetSecurityOrigin());
    if (cors_status != CrossOriginAccessControl::kAccessAllowed) {
      StringBuilder builder;
      builder.Append(
          "Response to preflight request doesn't pass access "
          "control check: ");
      CrossOriginAccessControl::AccessControlErrorString(
          builder, cors_status, response, GetSecurityOrigin(),
  request_context_); HandlePreflightFailure(response.Url().GetString(),
  builder.ToString()); return;
    }

    CrossOriginAccessControl::PreflightStatus preflight_status =
        CrossOriginAccessControl::CheckPreflight(response);
    if (preflight_status != CrossOriginAccessControl::kPreflightSuccess) {
      StringBuilder builder;
      CrossOriginAccessControl::PreflightErrorString(builder, preflight_status,
                                                     response);
      HandlePreflightFailure(response.Url().GetString(), builder.ToString());
      return;
    }

    if (actual_request_.IsExternalRequest()) {
      CrossOriginAccessControl::PreflightStatus external_preflight_status =
          CrossOriginAccessControl::CheckExternalPreflight(response);
      if (external_preflight_status !=
          CrossOriginAccessControl::kPreflightSuccess) {
        StringBuilder builder;
        CrossOriginAccessControl::PreflightErrorString(
            builder, external_preflight_status, response);
        HandlePreflightFailure(response.Url().GetString(), builder.ToString());
        return;
      }
    }

    std::unique_ptr<CrossOriginPreflightResultCacheItem> preflight_result =
        WTF::WrapUnique(new CrossOriginPreflightResultCacheItem(
            actual_request_.GetFetchCredentialsMode()));
    if (!preflight_result->Parse(response, access_control_error_description) ||
        !preflight_result->AllowsCrossOriginMethod(
            actual_request_.HttpMethod(), access_control_error_description) ||
        !preflight_result->AllowsCrossOriginHeaders(
            actual_request_.HttpHeaderFields(),
            access_control_error_description)) {
      HandlePreflightFailure(response.Url().GetString(),
                             access_control_error_description);
      return;
    }

    if (IsMainThread()) {
      // TODO(horo): Currently we don't support the CORS preflight cache on
  worker
      // thread when off-main-thread-fetch is enabled. https://crbug.com/443374
      CrossOriginPreflightResultCache::Shared().AppendEntry(
          GetSecurityOrigin()->ToString(), actual_request_.Url(),
          std::move(preflight_result));
    }
  }
  */

  if (preflight_status ==
      CrossOriginAccessControl::PreflightStatus::kPreflightSuccess)
    delegate_->Resume();
  else
    delegate_->CancelWithError(net::ERR_ABORTED);
}

bool CORSURLLoaderThrottle::IsCORSEnabled() {
  return request_.fetch_request_mode == FETCH_REQUEST_MODE_CORS ||
         request_.fetch_request_mode ==
             FETCH_REQUEST_MODE_CORS_WITH_FORCED_PREFLIGHT ||
         request_.is_external_request_;
}

bool CORSURLLoaderThrottle::IsSameOrigin() {
  return request_.request_initiator.value().IsSameOriginWith(
      url::Origin(request_.url));
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

  // TODO(hintzed): How do we achieve the same thing here?
  /*
  if (!IsMainThread()) {
    // TODO(horo): Currently we don't support the CORS preflight cache on worker
    // thread when off-main-thread-fetch is enabled. See
    // https://crbug.com/443374.
    should_ignore_preflight_cache = true;
  } else {
    // Prevent use of the CORS preflight cache when instructed by the DevTools
    // not to use caches.
    probe::shouldForceCORSPreflight(GetDocument(),
                                    &should_ignore_preflight_cache);
  }*/

  if (should_ignore_preflight_cache ||
      !CrossOriginPreflightResultCache::Shared().CanSkipPreflight(
          request_.request_initiator.value().Serialize(), request_.url,
          request_.fetch_credentials_mode, request_.method, request_.headers)) {
    return false;
  }

  return true;
}

void CORSURLLoaderThrottle::StartPreflightRequest() {
  pending_preflights_++;

  ResourceRequest preflight_request =
      CrossOriginAccessControl::CreateAccessControlPreflightRequest(request_);

  std::vector<std::unique_ptr<URLLoaderThrottle>> throttles;

  loader_ = ThrottlingURLLoader::CreateLoaderAndStart(
      &url_loader_factory_, std::move(throttles), requestInfo_.routing_id,
      requestInfo_.request_id, requestInfo_.options, preflight_request,
      &client_,
      net::MutableNetworkTrafficAnnotationTag(NO_TRAFFIC_ANNOTATION_YET));
}

}  // namespace content
