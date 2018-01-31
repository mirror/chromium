// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_LOADER_SIGNED_EXCHANGE_CERT_CETCHER_H_
#define CONTENT_BROWSER_LOADER_SIGNED_EXCHANGE_CERT_CETCHER_H_

#include "mojo/common/data_pipe_drainer.h"
#include "services/network/public/interfaces/url_loader.mojom.h"

class GURL;

namespace network {
struct ResourceRequest;
}  // namespace network

namespace content {

class SharedURLLoaderFactory;
class URLLoaderThrottle;
class ThrottlingURLLoader;

class SignedExchangeCertFetcher : public network::mojom::URLLoaderClient,
                                  public mojo::common::DataPipeDrainer::Client {
 public:
  using CertVerifiedCallback =
      base::OnceCallback<void(net::CertStatus cert_staus,
                              const base::Optional<net::SSLInfo>& ssl_info)>;

  static std::unique_ptr<SignedExchangeCertFetcher> CreateAndStart(
      scoped_refptr<SharedURLLoaderFactory> shared_url_loader_factory,
      std::vector<std::unique_ptr<URLLoaderThrottle>> throttles,
      const GURL& cert_url,
      bool force_fetch,
      CertVerifiedCallback callback);
  ~SignedExchangeCertFetcher() override;

  // network::mojom::URLLoaderClient
  void OnReceiveResponse(
      const network::ResourceResponseHead& head,
      const base::Optional<net::SSLInfo>& ssl_info,
      network::mojom::DownloadedTempFilePtr downloaded_file) override;
  void OnReceiveRedirect(const net::RedirectInfo& redirect_info,
                         const network::ResourceResponseHead& head) override;
  void OnDataDownloaded(int64_t data_length, int64_t encoded_length) override;
  void OnUploadProgress(int64_t current_position,
                        int64_t total_size,
                        OnUploadProgressCallback callback) override;
  void OnReceiveCachedMetadata(const std::vector<uint8_t>& data) override;
  void OnTransferSizeUpdated(int32_t transfer_size_diff) override;
  void OnStartLoadingResponseBody(
      mojo::ScopedDataPipeConsumerHandle body) override;
  void OnComplete(const network::URLLoaderCompletionStatus& status) override;

  // mojo::Common::DataPipeDrainer::Client
  void OnDataAvailable(const void* data, size_t num_bytes) override;
  void OnDataComplete() override;

 private:
  SignedExchangeCertFetcher(
      scoped_refptr<SharedURLLoaderFactory> shared_url_loader_factory,
      std::vector<std::unique_ptr<URLLoaderThrottle>> throttles,
      const GURL& cert_url,
      bool force_fetch,
      CertVerifiedCallback callback);
  void Start();

  scoped_refptr<SharedURLLoaderFactory> shared_url_loader_factory_;
  std::vector<std::unique_ptr<URLLoaderThrottle>> throttles_;
  std::unique_ptr<network::ResourceRequest> resource_request_;
  const bool force_fetch_;
  CertVerifiedCallback callback_;

  std::unique_ptr<ThrottlingURLLoader> url_loader_;
  bool data_completed_ = false;
  std::unique_ptr<mojo::common::DataPipeDrainer> drainer_;
  std::string original_body_string_;

  DISALLOW_COPY_AND_ASSIGN(SignedExchangeCertFetcher);
};

}  // namespace content

#endif  // CONTENT_BROWSER_LOADER_SIGNED_EXCHANGE_CERT_CETCHER_H_
