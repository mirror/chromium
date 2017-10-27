// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_APPCACHE_APPCACHE_SUBRESOURCE_URL_FACTORY_H_
#define CONTENT_BROWSER_APPCACHE_APPCACHE_SUBRESOURCE_URL_FACTORY_H_

#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "content/browser/loader/url_loader_request_handler.h"
#include "content/browser/url_loader_factory_getter.h"
#include "content/common/content_export.h"
#include "content/public/common/url_loader_factory.mojom.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "url/gurl.h"

namespace content {

class AppCacheHost;
class AppCacheJob;
class AppCacheRequestHandler;
class AppCacheServiceImpl;

// Implements the URLLoaderFactory mojom for AppCache subresource requests.
class CONTENT_EXPORT AppCacheSubresourceURLFactory
    : public mojom::URLLoaderFactory {
 public:
  ~AppCacheSubresourceURLFactory() override;

  // Factory function to create an instance of the factory.
  // 1. The |factory_getter| parameter is used to query the network service
  //    to pass network requests to.
  // 2. The |host| parameter contains the appcache host instance. This is used
  //    to create the AppCacheRequestHandler instances for handling subresource
  //    requests.
  static void CreateURLLoaderFactory(
      URLLoaderFactoryGetter* factory_getter,
      base::WeakPtr<AppCacheHost> host,
      mojom::URLLoaderFactoryPtr* loader_factory);

  // mojom::URLLoaderFactory implementation.
  void CreateLoaderAndStart(mojom::URLLoaderRequest url_loader_request,
                            int32_t routing_id,
                            int32_t request_id,
                            uint32_t options,
                            const ResourceRequest& request,
                            mojom::URLLoaderClientPtr client,
                            const net::MutableNetworkTrafficAnnotationTag&
                                traffic_annotation) override;
  void Clone(mojom::URLLoaderFactoryRequest request) override;

  base::WeakPtr<AppCacheSubresourceURLFactory> GetWeakPtr();

 private:
  friend class AppCacheNetworkServiceBrowserTest;
  friend class AppCacheSubresourceLoaderTest;
  FRIEND_TEST_ALL_PREFIXES(AppCacheSubresourceLoaderTest,
                           CreateSubresourceLoader);

  // URLLoader implementation that utilizes either a network loader
  // or an appcache loader depending on where the resources should
  // be loaded from. This class binds to the remote client in the
  // renderer and internally creates one or the other kind of loader.
  // The URLLoader and URLLoaderClient interfaces are proxied between
  // the remote consumer and the chosen internal loader.
  //
  // This class owns and scopes the lifetime of the AppCacheRequestHandler
  // for the duration of a subresource load.
  class CONTENT_EXPORT SubresourceLoader : public mojom::URLLoader,
                                           public mojom::URLLoaderClient {
   public:
    SubresourceLoader(mojom::URLLoaderRequest url_loader_request,
                      int32_t routing_id,
                      int32_t request_id,
                      uint32_t options,
                      const ResourceRequest& request,
                      mojom::URLLoaderClientPtr client,
                      const net::MutableNetworkTrafficAnnotationTag& annotation,
                      base::WeakPtr<AppCacheHost> appcache_host,
                      scoped_refptr<URLLoaderFactoryGetter> net_factory_getter);
    ~SubresourceLoader() override;

    void SetHandlerForTesting(std::unique_ptr<AppCacheRequestHandler> handler);

   private:
    friend class AppCacheSubresourceLoaderTest;

    void OnConnectionError();

    void Start();
    void ContinueStart(StartLoaderCallback start_function);
    void CreateAndStartAppCacheLoader(StartLoaderCallback start_function);
    void CreateAndStartNetworkLoader();

    // mojom::URLLoader implementation
    // Called by the remote client in the renderer.
    void FollowRedirect() override;
    void ContinueFollowRedirect(StartLoaderCallback start_function);
    void SetPriority(net::RequestPriority priority,
                     int32_t intra_priority_value) override;
    void PauseReadingBodyFromNet() override;
    void ResumeReadingBodyFromNet() override;

    // mojom::URLLoaderClient implementation
    // Called by either the appcache or network loader, whichever is in use.
    void OnReceiveResponse(
        const ResourceResponseHead& response_head,
        const base::Optional<net::SSLInfo>& ssl_info,
        mojom::DownloadedTempFilePtr downloaded_file) override;
    void ContinueOnReceiveResponse(const ResourceResponseHead& response_head,
                                   const base::Optional<net::SSLInfo>& ssl_info,
                                   mojom::DownloadedTempFilePtr downloaded_file,
                                   StartLoaderCallback start_function);

    void OnReceiveRedirect(const net::RedirectInfo& redirect_info,
                           const ResourceResponseHead& response_head) override;
    void ContinueOnReceiveRedirect(const ResourceResponseHead& response_head,
                                   StartLoaderCallback start_function);

    void OnDataDownloaded(int64_t data_len, int64_t encoded_data_len) override;
    void OnUploadProgress(int64_t current_position,
                          int64_t total_size,
                          OnUploadProgressCallback ack_callback) override;
    void OnReceiveCachedMetadata(const std::vector<uint8_t>& data) override;
    void OnTransferSizeUpdated(int32_t transfer_size_diff) override;
    void OnStartLoadingResponseBody(
        mojo::ScopedDataPipeConsumerHandle body) override;

    void OnComplete(const ResourceRequestCompletionStatus& status) override;
    void ContinueOnComplete(const ResourceRequestCompletionStatus& status,
                            StartLoaderCallback start_function);

    // Max number of http redirects to follow. The Fetch spec says: "If
    // request's redirect count is twenty, return a network error."
    // https://fetch.spec.whatwg.org/#http-redirect-fetch
    static constexpr int kMaxRedirects = 20;

    // The binding and client pointer associated with the renderer.
    mojo::Binding<mojom::URLLoader> remote_binding_;
    mojom::URLLoaderClientPtr remote_client_;

    ResourceRequest request_;
    int32_t routing_id_;
    int32_t request_id_;
    uint32_t options_;
    net::MutableNetworkTrafficAnnotationTag traffic_annotation_;
    scoped_refptr<URLLoaderFactoryGetter> network_loader_factory_;
    net::RedirectInfo redirect_info_;
    int redirect_limit_ = kMaxRedirects;
    bool did_receive_network_response_ = false;
    bool has_paused_reading_ = false;
    bool has_set_priority_ = false;
    net::RequestPriority priority_;
    int32_t intra_priority_value_;

    // Core appcache logic that decides how to handle a request.
    std::unique_ptr<AppCacheRequestHandler> handler_;

    // The local binding to either our network or appcache loader,
    // we only use one of them at any given time.
    mojo::Binding<mojom::URLLoaderClient> local_client_binding_;
    mojom::URLLoaderPtr network_loader_;
    mojom::URLLoaderPtr appcache_loader_;

    base::WeakPtr<AppCacheHost> host_;

    base::WeakPtrFactory<SubresourceLoader> weak_factory_;
    DISALLOW_COPY_AND_ASSIGN(SubresourceLoader);
  };

  AppCacheSubresourceURLFactory(URLLoaderFactoryGetter* factory_getter,
                                base::WeakPtr<AppCacheHost> host);
  void OnConnectionError();

  mojo::BindingSet<mojom::URLLoaderFactory> bindings_;
  scoped_refptr<URLLoaderFactoryGetter> default_url_loader_factory_getter_;
  base::WeakPtr<AppCacheHost> appcache_host_;
  base::WeakPtrFactory<AppCacheSubresourceURLFactory> weak_factory_;
  DISALLOW_COPY_AND_ASSIGN(AppCacheSubresourceURLFactory);
};

}  // namespace content

#endif  // CONTENT_BROWSER_APPCACHE_APPCACHE_URL_LOADER_FACTORY_H_
