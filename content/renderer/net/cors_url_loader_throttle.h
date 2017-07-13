// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_NET_CORS_URL_LOADER_THROTTLE_H_
#define CONTENT_RENDERER_NET_CORS_URL_LOADER_THROTTLE_H_

#include "content/common/throttling_url_loader.h"
#include "content/common/url_loader_factory.mojom.h"
#include "content/public/common/resource_response.h"
#include "content/public/common/url_loader_throttle.h"
#include "content/renderer/net/cors_preflight_url_loader_client.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "net/url_request/redirect_info.h"

namespace content {

// Max number of CORS redirects handled in DocumentThreadableLoader. Same number
// as net/url_request/url_request.cc, and same number as
// https://fetch.spec.whatwg.org/#concept-http-fetch, Step 4.
// FIXME: currently the number of redirects is counted and limited here and in
// net/url_request/url_request.cc separately.
static const int kMaxCORSRedirects = 20;

class CONTENT_EXPORT CORSURLLoaderThrottle
    : public content::URLLoaderThrottle,
      public content::CORSPreflightCallback {
 public:
  struct RequestInfo {
    int32_t routing_id;
    int32_t request_id;
    uint32_t options;
  };

  CORSURLLoaderThrottle(RequestInfo& requestInfo,
                        ResourceRequest& url_request,
                        mojom::URLLoaderFactory& url_loader_factory);

  ~CORSURLLoaderThrottle() override;

  // content::URLLoaderThrottle implementation.
  void WillStartRequest(const GURL& url,
                        int load_flags,
                        content::ResourceType resource_type,
                        bool* defer) override;

  void WillRedirectRequest(const net::RedirectInfo& redirect_info,
                           const ResourceResponseHead& response,
                           bool* defer) override;

  void WillProcessResponse(const ResourceResponseHead& response,
                           bool* defer) override;

  void OnPreflightResponse(scoped_refptr<ResourceResponse>) override;

  void OnPreflightRedirect(const net::RedirectInfo&,
                           const ResourceResponseHead&) override;

 private:
  bool IsSameOrigin();
  bool IsNavigation();
  bool IsCORSEnabled();
  bool IsCORSAllowed();
  bool NeedsPreflight();
  void StartPreflightRequest();
  bool IsAllowedRedirect(const GURL& url) const;
  ResourceRequest CreateAccessControlPreflightRequest(const ResourceRequest&);
  url::Origin GetSecurityOrigin() const;

  // Max number of times that this CORSURLLoaderThrottle can follow
  // cross-origin redirects. This is used to limit the number of redirects. But
  // this value is not the max number of total redirects allowed, because
  // same-origin redirects are not counted here.
  int cors_redirect_limit_;

  std::unique_ptr<url::Origin> security_origin_;

  bool cors_flag_;
  std::unique_ptr<ThrottlingURLLoader> loader_;
  RequestInfo& requestInfo_;
  ResourceRequest& request_;
  mojom::URLLoaderFactory& url_loader_factory_;
  CORSPreflightURLLoaderClient client_;
};

}  // namespace content

#endif  // CONTENT_RENDERER_NET_CORS_URL_LOADER_THROTTLE_H_
