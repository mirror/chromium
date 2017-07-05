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

class CONTENT_EXPORT CORSURLLoaderThrottle
    : public content::URLLoaderThrottle,
      public content::CORSPreflightCallback {
 public:
  struct RequestInfo {
    int32_t routing_id;
    int32_t request_id;
    uint32_t options;
  };

  CORSURLLoaderThrottle(
      RequestInfo& requestInfo,
      ResourceRequest& url_request,
      mojom::URLLoaderFactory& url_loader_factory,
      scoped_refptr<base::SingleThreadTaskRunner> loading_task_runner);

  ~CORSURLLoaderThrottle() override;

  // content::URLLoaderThrottle implementation.
  void WillStartRequest(const GURL& url,
                        int load_flags,
                        content::ResourceType resource_type,
                        bool* defer) override;

  void WillRedirectRequest(const net::RedirectInfo& redirect_info,
                           bool* defer) override;

  void WillProcessResponse(const ResourceResponseHead& response,
                           bool* defer) override;

  void OnPreflightResponse(scoped_refptr<ResourceResponse>) override;

 private:
  bool IsSameOrigin();
  bool IsNavigation();
  bool IsCORSEnabled();
  bool IsCORSAllowed();

  bool NeedsPreflight();
  void StartPreflightRequest();
  ResourceRequest CreateAccessControlPreflightRequest(const ResourceRequest&);
  url::Origin GetSecurityOrigin() const;

  bool cors_flag_;
  std::unique_ptr<ThrottlingURLLoader> loader_;
  RequestInfo& requestInfo_;
  ResourceRequest& request_;
  mojom::URLLoaderFactory& url_loader_factory_;
  CORSPreflightURLLoaderClient client_;
};

}  // namespace content

#endif  // CONTENT_RENDERER_NET_CORS_URL_LOADER_THROTTLE_H_
