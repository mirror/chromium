// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_CHILD_LOADER_CORS_URL_LOADER_H_
#define CONTENT_CHILD_LOADER_CORS_URL_LOADER_H_

#include "content/public/common/url_loader_factory.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "net/traffic_annotation/network_traffic_annotation.h"

namespace content {

// Wrapper class that adds cross-origin resource sharing capabilities
// (https://fetch.spec.whatwg.org/#http-cors-protocol), delegating requests as
// well as potential preflight requests to the supplied
// |network_loader_factory|.
class CONTENT_EXPORT CORSURLLoader : public mojom::URLLoader,
                                     public mojom::URLLoaderClient {
 public:
  // Assumes network_loader_factory outlives this loader.
  CORSURLLoader(
      int32_t routing_id,
      int32_t request_id,
      uint32_t options,
      const ResourceRequest& resource_request,
      mojom::URLLoaderClientPtr client,
      const net::MutableNetworkTrafficAnnotationTag& traffic_annotation,
      mojom::URLLoaderFactory* network_loader_factory);

  ~CORSURLLoader() override;

  // mojom::URLLoader overrides:

  void FollowRedirect() override;

  void SetPriority(net::RequestPriority priority,
                   int intra_priority_value) override;

  // mojom::URLLoaderClient overrides:

  // Perform CORS check and tainting.
  void OnReceiveResponse(const ResourceResponseHead& head,
                         const base::Optional<net::SSLInfo>& ssl_info,
                         mojom::DownloadedTempFilePtr downloaded_file) override;

  // Perform CORS check and adjust the request and proceed, or just terminate
  // the request (i.e. calling forwarding client's OnComplete with error
  // code).
  void OnReceiveRedirect(const net::RedirectInfo& redirect_info,
                         const ResourceResponseHead& head) override;

  void OnDataDownloaded(int64_t data_length, int64_t encoded_length) override;

  void OnUploadProgress(int64_t current_position,
                        int64_t total_size,
                        base::OnceCallback<void()> callback) override;

  void OnReceiveCachedMetadata(const std::vector<uint8_t>& data) override;

  void OnTransferSizeUpdated(int32_t transfer_size_diff) override;

  void OnStartLoadingResponseBody(
      mojo::ScopedDataPipeConsumerHandle body) override;

  void OnComplete(
      const ResourceRequestCompletionStatus& completion_status) override;

 private:
  enum class Mode { kUndefined, kPreflight, kActualRequest, kCORSFailed };

  void HandlePreflightResponse(const ResourceResponseHead& head);

  void HandlePreflightRedirect(const net::RedirectInfo& redirect_info,
                               const ResourceResponseHead& head);

  void HandleActualResponse(const ResourceResponseHead& head,
                            const base::Optional<net::SSLInfo>& ssl_info,
                            mojom::DownloadedTempFilePtr downloaded_file);

  void HandleActualRedirect(const net::RedirectInfo& redirect_info,
                            const ResourceResponseHead& head);

  bool IsSameOriginOrWhitelisted(const GURL& url) const;
  bool IsNavigation() const;
  bool IsCORSEnabled() const;
  bool IsCORSAllowed(std::string& error_description) const;
  bool NeedsPreflight() const;
  bool IsAllowedRedirect(const GURL& url) const;
  void AddOriginHeader(ResourceRequest& request);
  void StartPreflightRequest();
  void StartActualRequest();
  void DispatchRequest(const ResourceRequest& request,
                       const int32_t request_id);
  void DidFailAccessControlCheck(std::string error_description);
  ResourceRequest CreateAccessControlPreflightRequest(
      const ResourceRequest& request);

  // Notify Inspector and log to console about resource response. Use this
  // method if response is not going to be finished normally.
  void ReportResponseReceived(unsigned long identifier,
                              const ResourceResponseHead& head);

  Mode mode_;

  // Used to initiate the actual request (or any preflight requests)
  // with the default network loader factory.
  mojom::URLLoaderFactory* network_loader_factory_;

  // For the actual request:
  mojom::URLLoaderPtr network_loader_;
  std::unique_ptr<mojo::Binding<mojom::URLLoaderClient>>
      network_client_binding_;

  // To be a URLLoader for the client.
  mojom::URLLoaderClientPtr forwarding_client_;

  // Max number of times that this CORSURLLoaderThrottle can follow
  // cross-origin redirects. This is used to limit the number of redirects. But
  // this value is not the max number of total redirects allowed, because
  // same-origin redirects are not counted here.
  int cors_redirect_limit_;

  // Make a copy of the request because the reference passed to the constructor
  // does apparently not outlive this loader.
  ResourceRequest request_;

  const int32_t routing_id_;
  const int32_t request_id_;
  const uint32_t options_;

  const net::NetworkTrafficAnnotationTag traffic_annotation_;

  url::Origin security_origin_;

  bool cors_flag_;

  DISALLOW_COPY_AND_ASSIGN(CORSURLLoader);
};

}  // namespace content

#endif  // CONTENT_CHILD_LOADER_CORS_URL_LOADER_H_
