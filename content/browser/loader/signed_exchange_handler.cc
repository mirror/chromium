// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/signed_exchange_handler.h"

#include "base/feature_list.h"
#include "content/browser/loader/resource_requester_info.h"
#include "content/browser/loader/signed_exchange_cert_fetcher.h"
#include "content/browser/loader/url_loader_factory_impl.h"
#include "content/browser/url_loader_factory_getter.h"
#include "content/common/weak_wrapper_shared_url_loader_factory.h"
#include "content/public/browser/resource_context.h"
#include "content/public/common/content_features.h"
#include "content/public/common/url_loader_throttle.h"
#include "mojo/public/cpp/system/string_data_pipe_producer.h"
#include "net/base/io_buffer.h"
#include "net/http/http_response_headers.h"
#include "services/network/public/cpp/features.h"
#include "services/network/public/cpp/resource_response.h"
#include "services/network/public/cpp/url_loader_completion_status.h"

namespace content {

namespace {

void GetContextsCallbackForCertificateFetcherForSignedExchange(
    ResourceContext* resource_context,
    net::URLRequestContext* request_context,
    ResourceType resource_type,
    ResourceContext** resource_context_out,
    net::URLRequestContext** request_context_out) {
  *resource_context_out = resource_context;
  *request_context_out = request_context;
}

}  // namespace

SignedExchangeHandler::SignedExchangeHandler(
    std::unique_ptr<net::SourceStream> upstream,
    ExchangeHeadersCallback headers_callback,
    URLLoaderFactoryGetter* default_url_loader_factory_getter,
    ResourceContext* resource_context,
    net::URLRequestContext* request_context,
    URLLoaderThrottlesGetter url_loader_throttles_getter)
    : net::FilterSourceStream(net::SourceStream::TYPE_NONE,
                              std::move(upstream)),
      headers_callback_(std::move(headers_callback)),
      default_url_loader_factory_getter_(default_url_loader_factory_getter),
      resource_context_(resource_context),
      request_context_(request_context),
      url_loader_throttles_getter_(std::move(url_loader_throttles_getter)),
      weak_factory_(this) {
  DCHECK(base::FeatureList::IsEnabled(features::kSignedHTTPExchange));

  // Triggering the first read (asynchronously) for header parsing.
  header_out_buf_ = base::MakeRefCounted<net::IOBufferWithSize>(1);
  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&SignedExchangeHandler::DoHeaderLoop,
                                weak_factory_.GetWeakPtr()));
}

SignedExchangeHandler::~SignedExchangeHandler() = default;

int SignedExchangeHandler::FilterData(net::IOBuffer* output_buffer,
                                      int output_buffer_size,
                                      net::IOBuffer* input_buffer,
                                      int input_buffer_size,
                                      int* consumed_bytes,
                                      bool upstream_eof_reached) {
  *consumed_bytes = 0;

  original_body_string_.append(input_buffer->data(), input_buffer_size);
  *consumed_bytes += input_buffer_size;

  // We shouldn't write any data into the out buffer while we're
  // parsing the header.
  if (headers_callback_)
    return 0;

  if (upstream_eof_reached) {
    // Run the parser code if this part is run for the first time.
    // Now original_body_string_ has the entire body.
    // (Note that we may come here multiple times if output_buffer_size
    // is not big enough.
    // TODO(https://crbug.com/803774): Do the streaming instead.
    size_t size_to_copy =
        std::min(original_body_string_.size() - body_string_offset_,
                 base::checked_cast<size_t>(output_buffer_size));
    memcpy(output_buffer->data(),
           original_body_string_.data() + body_string_offset_, size_to_copy);
    body_string_offset_ += size_to_copy;
    return base::checked_cast<int>(size_to_copy);
  }

  return 0;
}

std::string SignedExchangeHandler::GetTypeAsString() const {
  return "HTXG";  // Tentative.
}

void SignedExchangeHandler::DoHeaderLoop() {
  // Run the internal read loop by ourselves until we finish
  // parsing the headers. (After that the caller should pumb
  // the Read() calls).
  DCHECK(headers_callback_);
  DCHECK(header_out_buf_);
  int rv = Read(header_out_buf_.get(), header_out_buf_->size(),
                base::BindRepeating(&SignedExchangeHandler::DidReadForHeaders,
                                    base::Unretained(this), false /* sync */));
  if (rv != net::ERR_IO_PENDING)
    DidReadForHeaders(true /* sync */, rv);
}

void SignedExchangeHandler::DidReadForHeaders(bool completed_syncly,
                                              int result) {
  if (MaybeRunHeadersCallback() || result < 0)
    return;
  DCHECK_EQ(0, result);
  if (completed_syncly) {
    base::SequencedTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(&SignedExchangeHandler::DoHeaderLoop,
                                  weak_factory_.GetWeakPtr()));
  } else {
    DoHeaderLoop();
  }
}

bool SignedExchangeHandler::MaybeRunHeadersCallback() {
  if (!headers_callback_)
    return false;

  // If this was the first read, fire the headers callback now.
  // TODO(https://crbug.com/803774): This is just for testing, we should
  // implement the CBOR parsing here.
  FillMockExchangeHeaders();

  // TODO(https://crbug.com/803774) Consume the bytes size that were
  // necessary to read out the headers.
  network::mojom::URLLoaderFactory* url_loader_factory = nullptr;
  if (base::FeatureList::IsEnabled(network::features::kNetworkService)) {
    DCHECK(default_url_loader_factory_getter_.get());
    url_loader_factory =
        default_url_loader_factory_getter_->GetNetworkFactory();
  } else {
    DCHECK(!default_url_loader_factory_getter_.get());
    url_loader_factory_impl_ = std::make_unique<URLLoaderFactoryImpl>(
        ResourceRequesterInfo::CreateForCertificateFetcherForSignedExchange(
            base::BindRepeating(
                &GetContextsCallbackForCertificateFetcherForSignedExchange,
                base::Unretained(resource_context_),
                base::Unretained(request_context_))));
    url_loader_factory = url_loader_factory_impl_.get();
  }
  DCHECK(url_loader_factory);
  DCHECK(url_loader_throttles_getter_);
  std::vector<std::unique_ptr<URLLoaderThrottle>> throttles =
      std::move(url_loader_throttles_getter_).Run();
  cert_fetcher_ = SignedExchangeCertFetcher::CreateAndStart(
      base::MakeRefCounted<WeakWrapperSharedURLLoaderFactory>(
          url_loader_factory),
      request_context_, std::move(throttles),
      GURL("http://127.0.0.1:8000/loading/htxg/resources/example.com.cert_msg"),
      false,
      base::BindOnce(&SignedExchangeHandler::OnCertVerified,
                     base::Unretained(this)));

  return true;
}
void SignedExchangeHandler::OnCertVerified(
    net::CertStatus cert_staus,
    const base::Optional<net::SSLInfo>& ssl_info) {
  LOG(ERROR) << "SignedExchangeHandler::OnCertVerified cert_staus: "
             << cert_staus;
  std::move(headers_callback_)
      .Run(request_url_, request_method_, response_head_, ssl_info);
}

void SignedExchangeHandler::FillMockExchangeHeaders() {
  // TODO(https://crbug.com/803774): Get the request url by parsing CBOR format.
  request_url_ = GURL("https://example.com/test.html");
  // TODO(https://crbug.com/803774): Get the request method by parsing CBOR
  // format.
  request_method_ = "GET";
  // TODO(https://crbug.com/803774): Get more headers by parsing CBOR.
  scoped_refptr<net::HttpResponseHeaders> headers(
      new net::HttpResponseHeaders("HTTP/1.1 200 OK"));
  response_head_.headers = headers;
  response_head_.mime_type = "text/html";
}

}  // namespace content
