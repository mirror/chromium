// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_ORIGIN_MANIFEST_ORIGIN_MANIFEST_URL_LOADER_THROTTLE_H_
#define CONTENT_BROWSER_ORIGIN_MANIFEST_ORIGIN_MANIFEST_URL_LOADER_THROTTLE_H_

#include <memory>

#include "base/callback.h"
#include "base/macros.h"
#include "content/public/common/network_service.mojom.h"
#include "content/public/common/url_loader_throttle.h"

namespace content {
class SimpleURLLoader;
}  // namespace content

class OriginManifestURLLoaderThrottle : public content::URLLoaderThrottle {
 public:
  ~OriginManifestURLLoaderThrottle() override;

  static std::unique_ptr<OriginManifestURLLoaderThrottle> Create();

  // content::URLLoaderThrottle implementation.
  void WillProcessResponse(const GURL& response_url,
                           const content::ResourceResponseHead& response_head,
                           bool* defer) override;

 private:
  OriginManifestURLLoaderThrottle();

  // Callback for when SimpleURLLoader download finishes. Used as a
  // BodyAsStringCallback as defined in SimpleURLLoader.
  void OnDownloadFinished(std::unique_ptr<std::string> response_body);

  std::unique_ptr<content::SimpleURLLoader> url_loader_;
  std::unique_ptr<content::mojom::NetworkService> network_service_;
  content::mojom::NetworkContextPtr network_context_;
  content::mojom::URLLoaderFactoryPtr url_loader_factory_;

  DISALLOW_COPY_AND_ASSIGN(OriginManifestURLLoaderThrottle);
};

#endif  // CONTENT_BROWSER_ORIGIN_MANIFEST_ORIGIN_MANIFEST_URL_LOADER_THROTTLE_H_
