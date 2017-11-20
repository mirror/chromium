// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_LOADER_WEBPACKAGE_LOADER_H_
#define CONTENT_BROWSER_LOADER_WEBPACKAGE_LOADER_H_

#include "base/memory/ref_counted.h"
#include "content/browser/loader/url_loader_request_handler.h"

namespace content {

class URLLoaderFactoryGetter;

class WebPackageRequestHandler : public URLLoaderRequestHandler {
 public:
  WebPackageRequestHandler(
      const ResourceRequest& resource_request,
      const scoped_refptr<URLLoaderFactoryGetter>&
          default_url_loader_factory_getter);
  ~WebPackageRequestHandler() override;

  void MaybeCreateLoader(const ResourceRequest& resource_request,
                         ResourceContext* resource_context,
                         LoaderCallback callback) override;
  bool MaybeCreateLoaderForResponse(
      const ResourceResponseHead& response,
      mojom::URLLoaderPtr* loader,
      mojom::URLLoaderClientRequest* client_request) override;

 private:
  ResourceRequest resource_request_;
  scoped_refptr<URLLoaderFactoryGetter> default_url_loader_factory_getter_;

  DISALLOW_COPY_AND_ASSIGN(WebPackageRequestHandler);
};

class WebPackageResourceLoader : public mojom::URLLoader,
                                 public mojom::URLLoaderClient {
 public:
  WebPackageResourceLoader(
      LoaderCallback loader_callback,
      const ResourceRequest& resource_request,
      scoped_refptr<URLLoaderFactoryGetter> url_loader_factory_getter);
  ~WebPackageResourceLoader() override;

  // mojom::URLLoader:
  void FollowRedirect() override;
  void SetPriority(net::RequestPriority priority,
                   int32_t intra_priority_value) override;
  void PauseReadingBodyFromNet() override;
  void ResumeReadingBodyFromNet() override;

  // mojom::URLLoaderClient for reading data from the network.
  void OnReceiveResponse(const ResourceResponseHead& response_head,
                         const base::Optional<net::SSLInfo>& ssl_info,
                         mojom::DownloadedTempFilePtr downloaded_file) override;
  void OnReceiveRedirect(const net::RedirectInfo& redirect_info,
                         const ResourceResponseHead& response_head) override;
  void OnDataDownloaded(int64_t data_len, int64_t encoded_data_len) override;
  void OnUploadProgress(int64_t current_position,
                        int64_t total_size,
                        OnUploadProgressCallback ack_callback) override;
  void OnReceiveCachedMetadata(const std::vector<uint8_t>& data) override;
  void OnTransferSizeUpdated(int32_t transfer_size_diff) override;
  void OnStartLoadingResponseBody(
      mojo::ScopedDataPipeConsumerHandle body) override;
  void OnComplete(const network::URLLoaderStatus& status) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(WebPackageResourceLoader);
};


}  // namespace content

#endif  // CONTENT_BROWSER_LOADER_WEBPACKAGE_LOADER_H_
