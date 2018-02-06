// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_LOADER_SIGNED_EXCHANGE_HANDLER_H_
#define CONTENT_BROWSER_LOADER_SIGNED_EXCHANGE_HANDLER_H_

#include <string>

#include "base/callback.h"
#include "base/optional.h"
#include "content/public/common/resource_type.h"
#include "mojo/public/cpp/system/data_pipe.h"
#include "net/base/completion_callback.h"
#include "net/cert/cert_status_flags.h"
#include "net/cert/cert_verifier.h"
#include "net/filter/filter_source_stream.h"
#include "net/log/net_log_with_source.h"
#include "net/ssl/ssl_info.h"
#include "services/network/public/cpp/resource_response.h"
#include "services/network/public/interfaces/url_loader_factory.mojom.h"
#include "url/gurl.h"

namespace net {
class SourceStream;
class URLRequestContext;
class X509Certificate;
}  // namespace net

namespace content {

class SignedExchangeCertFetcher;
class MerkleIntegritySourceStream;

// IMPORTANT: Currenly SignedExchangeHandler doesn't implement any verifying
// logic.
// TODO(https://crbug.com/803774): Implement verifying logic.
class SignedExchangeHandler final {
 public:
  // TODO(https://crbug.com/803774): Add verification status here.
  using ExchangeHeadersCallback =
      base::OnceCallback<void(net::Error error,
                              const GURL& request_url,
                              const std::string& request_method,
                              const network::ResourceResponseHead&,
                              std::unique_ptr<net::SourceStream> payload_stream,
                              base::Optional<net::SSLInfo>)>;

  // Once constructed |this| starts reading the |body| and parses the response
  // as a signed HTTP exchange. The response body of the exchange can be read
  // from |payload_stream| passed to |headers_callback|.
  SignedExchangeHandler(
      std::unique_ptr<net::SourceStream> body,
      ExchangeHeadersCallback headers_callback,
      network::mojom::URLLoaderFactoryPtrInfo url_loader_factory_for_browser,
      net::URLRequestContext* request_context);
  ~SignedExchangeHandler();

 private:
  void ReadLoop();
  void DidRead(bool completed_syncly, int result);
  bool RunHeadersCallback();
  void RunErrorCallback(net::Error);

  void OnCertRecieved(scoped_refptr<net::X509Certificate> cert);
  void OnCertVerifyComplete(int result);

  // Signed exchange contents.
  GURL request_url_;
  std::string request_method_;
  network::ResourceResponseHead response_head_;

  ExchangeHeadersCallback headers_callback_;
  std::unique_ptr<net::SourceStream> source_;

  // TODO(https://crbug.cxom/803774): Just for now. Implement the streaming
  // parser.
  scoped_refptr<net::IOBufferWithSize> read_buf_;
  std::string original_body_string_;

  network::mojom::URLLoaderFactoryPtr url_loader_factory_for_browser_ptr_;
  net::URLRequestContext* request_context_;
  std::unique_ptr<SignedExchangeCertFetcher> cert_fetcher_;

  std::unique_ptr<MerkleIntegritySourceStream> mi_stream_;
  scoped_refptr<net::X509Certificate> unverified_cert_;
  std::unique_ptr<net::CertVerifier::Request> cert_verifier_request_;
  std::unique_ptr<net::CertVerifyResult> cert_verify_result_;
  net::NetLogWithSource net_log_;

  base::WeakPtrFactory<SignedExchangeHandler> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(SignedExchangeHandler);
};

}  // namespace content

#endif  // CONTENT_BROWSER_LOADER_SIGNED_EXCHANGE_HANDLER_H_
