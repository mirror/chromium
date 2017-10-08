// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ORIGIN_MANIFEST_ORIGIN_MANIFEST_URL_LOADER_THROTTLE_H_
#define CHROME_BROWSER_ORIGIN_MANIFEST_ORIGIN_MANIFEST_URL_LOADER_THROTTLE_H_

#include <memory>

#include "base/macros.h"
#include "content/public/common/url_loader_throttle.h"
#include "net/url_request/url_fetcher_delegate.h"

namespace content {
class ResourceContext;
}

namespace net {
class URLFetcher;
}

namespace url {
class Origin;
}

class OriginManifestURLLoaderThrottle : public content::URLLoaderThrottle,
                                        net::URLFetcherDelegate {
 public:
  ~OriginManifestURLLoaderThrottle() override;

  static std::unique_ptr<OriginManifestURLLoaderThrottle> Create(
      content::ResourceContext* resource_context);

  // content::URLLoaderThrottle implementation.
  void WillStartRequest(const content::ResourceRequest& request,
                        bool* defer) override;
  void WillRedirectRequest(const net::RedirectInfo& redirect_info,
                           bool* defer) override;
  void WillProcessResponse(const content::ResourceResponseHead* response_head,
                           bool* defer) override;

  // net::URLFetcherDelegate implementation.
  void OnURLFetchComplete(const net::URLFetcher* source) override;

 protected:
  OriginManifestURLLoaderThrottle(content::ResourceContext* resource_context);

  content::ResourceContext* resource_context_;
  std::unique_ptr<url::Origin> origin_;
  std::unique_ptr<net::URLFetcher> url_fetcher_;

  DISALLOW_COPY_AND_ASSIGN(OriginManifestURLLoaderThrottle);
};

#endif  // CHROME_BROWSER_ORIGIN_MANIFEST_ORIGIN_MANIFEST_URL_LOADER_THROTTLE_H_
