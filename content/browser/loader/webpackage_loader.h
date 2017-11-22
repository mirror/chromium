// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_LOADER_WEBPACKAGE_LOADER_H_
#define CONTENT_BROWSER_LOADER_WEBPACKAGE_LOADER_H_

#include "base/memory/ref_counted.h"
#include "base/optional.h"
#include "content/browser/loader/url_loader_request_handler.h"
#include "content/browser/loader/webpackage_reader_adapter.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/system/data_pipe.h"
#include "services/network/public/cpp/url_loader_completion_status.h"

namespace content {

class URLLoaderFactoryGetter;
class WebPackageLoader;
struct ResourceRequest;

class WebPackageRequestHandler : public URLLoaderRequestHandler {
 public:
  WebPackageRequestHandler(
      const ResourceRequest& resource_request,
      const scoped_refptr<URLLoaderFactoryGetter>& url_loader_factory_getter);
  ~WebPackageRequestHandler() override;

  void MaybeCreateLoader(const ResourceRequest& resource_request,
                         ResourceContext* resource_context,
                         LoaderCallback callback) override;
  base::Optional<SubresourceLoaderParams> MaybeCreateSubresourceLoaderParams()
      override;
  bool MaybeCreateLoaderForResponse(
      const ResourceResponseHead& response,
      mojom::URLLoaderPtr* loader,
      mojom::URLLoaderClientRequest* client_request) override;

 private:
  ResourceRequest resource_request_;
  scoped_refptr<URLLoaderFactoryGetter> url_loader_factory_getter_;

  mojom::URLLoaderFactoryPtr subresource_loader_factory_;

  // Hold the package loader until the main resource load starts.
  std::unique_ptr<WebPackageLoader> webpackage_loader_;

  DISALLOW_COPY_AND_ASSIGN(WebPackageRequestHandler);
};

//-----------------------------------------------------------------------------

// This main loader is kept around for the entire lifetime of the frame.
// This class is self-destruct.
class WebPackageLoader : public mojom::URLLoader,
                         public mojom::URLLoaderClient,
                         public WebPackageReaderAdapterClient {
 public:
  WebPackageLoader(
      LoaderCallback loader_callback,
      const ResourceRequest& resource_request,
      scoped_refptr<URLLoaderFactoryGetter> url_loader_factory_getter);
  ~WebPackageLoader() override;

  void DestructIfNecessary();

  bool IsWebPackageRequest(const ResourceRequest& request) const {
    return (request.method == "GET" && request.url == webpackage_start_url_);
  }

  base::WeakPtr<WebPackageLoader> GetWeakPtr() {
    return weak_factory_.GetWeakPtr();
  }

  // For main-resource load.
  void CreateMainResourceLoader(mojom::URLLoaderRequest loader_request,
                                mojom::URLLoaderClientPtr loader_client);

  // For subresource load.
  mojom::URLLoaderFactoryPtr CreateSubresourceLoaderFactory();
  void CreateSubresourceLoader(
      mojom::URLLoaderRequest loader_request,
      int32_t routing_id,
      int32_t request_id,
      uint32_t options,
      const ResourceRequest& resource_request,
      mojom::URLLoaderClientPtr loader_client,
      const net::MutableNetworkTrafficAnnotationTag& traffic_annotation,
      bool allow_fallback = false);

 private:
  class MainResourceLoader;
  class SubresourceLoader;
  class SubresourceLoaderFactory;

  // WebPackageReaderAdapterClient:
  void OnFoundManifest(const WebPackageManifest& manifest) override;
  void OnFoundRequest(const ResourceRequest& request) override;

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
  void OnComplete(const network::URLLoaderCompletionStatus& status) override;

  void StartRedirectResponse(mojom::URLLoaderRequest request,
                             mojom::URLLoaderClientPtr client);
  void OnFinishMainResourceLoad();
  void OnFinishSubresourceLoad();

  LoaderCallback loader_callback_;
  const ResourceRequest webpackage_request_;
  GURL webpackage_start_url_;
  scoped_refptr<URLLoaderFactoryGetter> url_loader_factory_getter_;

  // URLLoader and client binding for the network loader.
  mojom::URLLoaderPtr network_loader_;
  mojo::Binding<mojom::URLLoaderClient> network_client_binding_;

  std::unique_ptr<WebPackageReaderAdapter> reader_;
  std::unique_ptr<MainResourceLoader> main_resource_loader_;
  std::unique_ptr<SubresourceLoaderFactory> subresource_loader_factory_;

  base::WeakPtrFactory<WebPackageLoader> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(WebPackageLoader);
};

}  // namespace content

#endif  // CONTENT_BROWSER_LOADER_WEBPACKAGE_LOADER_H_
