// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_LOADER_SIGNED_EXCHANGE_CERT_CETCHER_H_
#define CONTENT_BROWSER_LOADER_SIGNED_EXCHANGE_CERT_CETCHER_H_

#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/callback_helpers.h"
#include "base/optional.h"
#include "base/strings/string_piece_forward.h"
#include "content/common/content_export.h"
#include "mojo/common/data_pipe_drainer.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/network/public/interfaces/url_loader.mojom.h"

namespace net {
class X509Certificate;
}

namespace content {

class SharedURLLoaderFactory;

class CONTENT_EXPORT SignedExchangeCertFetcher
    : public network::mojom::URLLoaderClient,
      public mojo::common::DataPipeDrainer::Client {
 public:
  using CertificateCallback =
      base::OnceCallback<void(scoped_refptr<net::X509Certificate>)>;

  // Starts fetching the certificate using the |shared_url_loader_factory|.
  // The |callback| will be called with the certificate if succeeded. Otherwise
  // it will be called with null.
  static std::unique_ptr<SignedExchangeCertFetcher> CreateAndStart(
      scoped_refptr<SharedURLLoaderFactory> shared_url_loader_factory,
      const GURL& cert_url,
      bool force_fetch,
      CertificateCallback callback);

  // Parses a TLS 1.3 Certificate message containing X.509v3 certificates and
  // returns a vector of cert_data. Returns nullopt when failed to parse.
  static base::Optional<std::vector<base::StringPiece>> GetCertChainFromMessage(
      base::StringPiece message);

  ~SignedExchangeCertFetcher() override;

 private:
  FRIEND_TEST_ALL_PREFIXES(SignedExchangeCertFetcherTest, MaxCertSize_Exceeds);
  FRIEND_TEST_ALL_PREFIXES(SignedExchangeCertFetcherTest, MaxCertSize_SameSize);
  FRIEND_TEST_ALL_PREFIXES(SignedExchangeCertFetcherTest,
                           MaxCertSize_MultipleChunked);
  FRIEND_TEST_ALL_PREFIXES(SignedExchangeCertFetcherTest,
                           MaxCertSize_ContentLengthCheck);

  static base::ScopedClosureRunner SetMaxCertSizeForTest(size_t max_cert_size);

  SignedExchangeCertFetcher(const GURL& cert_url,
                            bool force_fetch,
                            CertificateCallback callback);
  void Start(scoped_refptr<SharedURLLoaderFactory> shared_url_loader_factory);
  void Abort();

  void OnClientConnectionError();

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

  std::unique_ptr<network::ResourceRequest> resource_request_;
  CertificateCallback callback_;

  network::mojom::URLLoaderPtr url_loader_;
  mojo::Binding<network::mojom::URLLoaderClient> client_binding_;
  std::unique_ptr<mojo::common::DataPipeDrainer> drainer_;
  std::string body_string_;

  DISALLOW_COPY_AND_ASSIGN(SignedExchangeCertFetcher);
};

}  // namespace content

#endif  // CONTENT_BROWSER_LOADER_SIGNED_EXCHANGE_CERT_CETCHER_H_
