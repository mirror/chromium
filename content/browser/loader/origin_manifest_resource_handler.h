// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_LOADER_ORIGIN_MANIFEST_RESOURCE_HANDLER_H_
#define CONTENT_BROWSER_LOADER_ORIGIN_MANIFEST_RESOURCE_HANDLER_H_

#include "base/memory/ref_counted.h"
#include "content/browser/loader/layered_resource_handler.h"
#include "content/common/content_export.h"
#include "net/url_request/url_fetcher_delegate.h"

namespace content {
class OriginManifest;
class OriginManifestStore;
}  // namespace content

namespace net {
class HttpResponseHeaders;
}

namespace content {

class CONTENT_EXPORT OriginManifestResourceHandler
    : public LayeredResourceHandler,
      private net::URLFetcherDelegate {
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

 private:
  enum Event { UNKNOWN, ON_REDIRECT, ON_RESPONSE };

  bool MaybeFetchOriginManifest(net::HttpResponseHeaders* headers);
  void ApplyManifest(const OriginManifest* origin_manifest,
                     net::HttpResponseHeaders* headers);

  void OnURLFetchComplete(const net::URLFetcher* source) override;

  OriginManifestStore& store_;

  Event load_event;
  std::string origin;
  std::string manifest_version;
  std::unique_ptr<net::URLFetcher> url_fetcher_;

  scoped_refptr<ResourceResponse> response_;
  std::unique_ptr<net::RedirectInfo> redirect_info_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_LOADER_ORIGIN_MANIFEST_RESOURCE_HANDLER_H_
