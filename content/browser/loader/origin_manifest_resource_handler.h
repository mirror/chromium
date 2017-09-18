// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_LOADER_ORIGIN_MANIFEST_RESOURCE_HANDLER_H_
#define CONTENT_BROWSER_LOADER_ORIGIN_MANIFEST_RESOURCE_HANDLER_H_

#include "components/origin_manifest/interfaces/origin_manifest.mojom.h"
#include "content/browser/loader/layered_resource_handler.h"
#include "content/common/content_export.h"
#include "content/public/browser/resource_request_info.h"

namespace origin_manifest {
class OriginManifestStoreImpl;
}  // namespace origin_manifest

namespace content {

class BrowserContext;

class CONTENT_EXPORT OriginManifestResourceHandler
    : public LayeredResourceHandler {
 public:
  OriginManifestResourceHandler(net::URLRequest* request,
                                std::unique_ptr<ResourceHandler> next_handler);
  ~OriginManifestResourceHandler() override;

  // LayeredResourceHandler implementation.
  void OnWillStart(const GURL& url,
                   std::unique_ptr<ResourceController> controller) override;
  void OnRequestRedirected(
      const net::RedirectInfo& redirect_info,
      ResourceResponse* response,
      std::unique_ptr<ResourceController> controller) override;
  void OnResponseStarted(
      ResourceResponse* response,
      std::unique_ptr<ResourceController> controller) override;

  void OnWillStartContinue(BrowserContext* browser_context);
  void OnWillStartFinish(origin_manifest::mojom::OriginManifestPtr om);
  void OnRequestRedirectedContinue(
      origin_manifest::mojom::OriginManifestPtr om);
  void OnResponseStartedContinue(origin_manifest::mojom::OriginManifestPtr om);

  void Run(const ResourceRequestInfo::WebContentsGetter& web_contents_getter);

 private:
  origin_manifest::mojom::OriginManifestStorePtr store_;
  std::string origin_;

  // things to buffer while we get things from the store
  const GURL* url_;
  scoped_refptr<ResourceResponse> response_;
  std::unique_ptr<net::RedirectInfo> redirect_info_;

  DISALLOW_COPY_AND_ASSIGN(OriginManifestResourceHandler);
};

}  // namespace content

#endif  // CONTENT_BROWSER_LOADER_ORIGIN_MANIFEST_RESOURCE_HANDLER_H_
