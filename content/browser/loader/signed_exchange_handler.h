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
#include "url/gurl.h"

namespace net {
class URLRequestContext;
class X509Certificate;
}  // namespace net

namespace content {

class ResourceContext;
class SignedExchangeCertFetcher;
class URLLoaderFactoryGetter;
class URLLoaderFactoryImpl;
class URLLoaderThrottle;

// IMPORTANT: Currenly SignedExchangeHandler doesn't implement any CBOR parsing
// logic nor verifying logic. It just behaves as if the passed body is a signed
// HTTP exchange which contains a request to "https://example.com/test.html" and
// a response with a payload which is equal to the original body.
// TODO(https://crbug.com/803774): Implement CBOR parsing logic and verifying
// logic.
class SignedExchangeHandler final : public net::FilterSourceStream {
 public:
  // TODO(https://crbug.com/803774): Add verification status here.
  using ExchangeHeadersCallback =
      base::OnceCallback<void(const GURL& request_url,
                              const std::string& request_method,
                              const network::ResourceResponseHead&,
                              base::Optional<net::SSLInfo>)>;
  using URLLoaderThrottlesGetter = base::RepeatingCallback<
      std::vector<std::unique_ptr<content::URLLoaderThrottle>>()>;

  // Once constructed |this| starts reading the |body| and parses the response
  // as a signed HTTP exchange. The response body of the exchange can be read
  // from |this| as a net::SourceStream after |headers_callback| is called.
  SignedExchangeHandler(
      std::unique_ptr<net::SourceStream> body,
      ExchangeHeadersCallback headers_callback,
      URLLoaderFactoryGetter* default_url_loader_factory_getter,
      ResourceContext* resource_context,
      net::URLRequestContext* request_context,
      URLLoaderThrottlesGetter url_loader_throttles_getter);
  ~SignedExchangeHandler() override;

  // net::FilterSourceStream:
  int FilterData(net::IOBuffer* output_buffer,
                 int output_buffer_size,
                 net::IOBuffer* input_buffer,
                 int input_buffer_size,
                 int* consumed_bytes,
                 bool upstream_eof_reached) override;
  std::string GetTypeAsString() const override;

 private:
  void DoHeaderLoop();
  void DidReadForHeaders(bool completed_syncly, int result);
  bool MaybeRunHeadersCallback();

  void OnCertRecieved(scoped_refptr<net::X509Certificate> cert);
  void OnCertVerifyComplete(int result);

  // TODO(https://crbug.com/803774): Remove this.
  void FillMockExchangeHeaders();

  // Signed exchange contents.
  GURL request_url_;
  std::string request_method_;
  network::ResourceResponseHead response_head_;

  ExchangeHeadersCallback headers_callback_;

  // Internal IOBuffer used during reading the header. Note that during parsing
  // the header we don't really need the output buffer, but we still need to
  // give some > 0 buffer.
  scoped_refptr<net::IOBufferWithSize> header_out_buf_;

  // TODO(https://crbug.cxom/803774): Just for now. Implement the streaming
  // parser.
  std::string original_body_string_;
  size_t body_string_offset_ = 0;

  // Used only when NetworkService is enabled.
  scoped_refptr<URLLoaderFactoryGetter> default_url_loader_factory_getter_;
  // Used only when NetworkService is disabled to keep a URLLoaderFactoryImpl
  // for fetching certificate.
  std::unique_ptr<URLLoaderFactoryImpl> url_loader_factory_impl_;
  ResourceContext* resource_context_;
  net::URLRequestContext* request_context_;
  URLLoaderThrottlesGetter url_loader_throttles_getter_;
  std::unique_ptr<SignedExchangeCertFetcher> cert_fetcher_;

  scoped_refptr<net::X509Certificate> unverified_cert_;
  std::unique_ptr<net::CertVerifier::Request> cert_verifier_request_;
  std::unique_ptr<net::CertVerifyResult> cert_verify_result_;
  net::NetLogWithSource net_log_;

  base::WeakPtrFactory<SignedExchangeHandler> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(SignedExchangeHandler);
};

}  // namespace content

#endif  // CONTENT_BROWSER_LOADER_SIGNED_EXCHANGE_HANDLER_H_
