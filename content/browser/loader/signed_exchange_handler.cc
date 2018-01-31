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
#include "content/public/common/content_features.h"
#include "content/public/common/url_loader_throttle.h"
#include "mojo/public/cpp/system/string_data_pipe_producer.h"
#include "net/http/http_response_headers.h"
#include "services/network/public/cpp/features.h"
#include "services/network/public/cpp/resource_response.h"
#include "services/network/public/cpp/url_loader_completion_status.h"

namespace content {
namespace {

constexpr size_t kPipeSizeForSignedResponseBody = 65536;

}  // namespace

SignedExchangeHandler::SignedExchangeHandler(
    mojo::ScopedDataPipeConsumerHandle body,
    URLLoaderFactoryGetter* default_url_loader_factory_getter,
    URLLoaderThrottlesGetter url_loader_throttles_getter,
    const GetContextsCallback& get_contexts_callback)
    : body_(std::move(body)),
      default_url_loader_factory_getter_(default_url_loader_factory_getter),
      url_loader_throttles_getter_(std::move(url_loader_throttles_getter)),
      get_contexts_callback_(get_contexts_callback) {
  DCHECK(base::FeatureList::IsEnabled(features::kSignedHTTPExchange));
}

SignedExchangeHandler::~SignedExchangeHandler() = default;

void SignedExchangeHandler::GetHTTPExchange(
    ExchangeFoundCallback found_callback,
    ExchangeFinishedCallback finish_callback) {
  DCHECK(!found_callback_);
  DCHECK(!finish_callback_);
  found_callback_ = std::move(found_callback);
  finish_callback_ = std::move(finish_callback);

  drainer_.reset(new mojo::common::DataPipeDrainer(this, std::move(body_)));
}

void SignedExchangeHandler::OnDataAvailable(const void* data,
                                            size_t num_bytes) {
  original_body_string_.append(static_cast<const char*>(data), num_bytes);
}

void SignedExchangeHandler::OnDataComplete() {
  if (!found_callback_)
    return;

  // TODO(https://crbug.com/803774): Get the request url by parsing CBOR format.
  GURL request_url = GURL("https://example.com/test.html");
  // TODO(https://crbug.com/803774): Get the request method by parsing CBOR
  // format.
  std::string request_method = "GET";
  // TODO(https://crbug.com/803774): Get the payload by parsing CBOR format.
  std::string payload = original_body_string_;
  original_body_string_.clear();

  // TODO(https://crbug.com/803774): Get more headers by parsing CBOR.
  scoped_refptr<net::HttpResponseHeaders> headers(
      new net::HttpResponseHeaders("HTTP/1.1 200 OK"));
  network::ResourceResponseHead resource_response;
  resource_response.headers = headers;
  resource_response.mime_type = "text/html";

  // TODO(https://crbug.com/803774): Get the certificate by parsing CBOR and
  // verify.
  base::Optional<net::SSLInfo> ssl_info;

  network::mojom::URLLoaderFactory* url_loader_factory = nullptr;
  if (base::FeatureList::IsEnabled(network::features::kNetworkService)) {
    DCHECK(default_url_loader_factory_getter_.get());
    url_loader_factory =
        default_url_loader_factory_getter_->GetNetworkFactory();
  } else {
    DCHECK(!default_url_loader_factory_getter_.get());
    url_loader_factory_impl_ = std::make_unique<URLLoaderFactoryImpl>(
        ResourceRequesterInfo::CreateForCertificateFetcherForSignedExchange(
            get_contexts_callback_));
    url_loader_factory = url_loader_factory_impl_.get();
  }
  DCHECK(url_loader_factory);
  DCHECK(url_loader_throttles_getter_);

  std::vector<std::unique_ptr<URLLoaderThrottle>> throttles =
      std::move(url_loader_throttles_getter_).Run();
  cert_fetcher_ = SignedExchangeCertFetcher::CreateAndStart(
      base::MakeRefCounted<WeakWrapperSharedURLLoaderFactory>(
          url_loader_factory),
      std::move(throttles), GURL("http://127.0.0.1:8000/test.cert"), false);

  mojo::DataPipe pipe(kPipeSizeForSignedResponseBody);
  // TODO(https://crbug.com/803774): Write the error handling code. This could
  // fail.
  DCHECK(pipe.consumer_handle.is_valid());
  mojo::ScopedDataPipeConsumerHandle response_body =
      std::move(pipe.consumer_handle);
  std::move(found_callback_)
      .Run(request_url, request_method, resource_response, std::move(ssl_info),
           std::move(response_body));

  data_producer_ = std::make_unique<mojo::StringDataPipeProducer>(
      std::move(pipe.producer_handle));
  data_producer_->Write(payload,
                        base::BindOnce(&SignedExchangeHandler::OnDataWritten,
                                       base::Unretained(this)));
}

void SignedExchangeHandler::OnDataWritten(MojoResult result) {
  data_producer_.reset();
  std::move(finish_callback_)
      .Run(network::URLLoaderCompletionStatus(
          result == MOJO_RESULT_OK ? net::OK : net::ERR_FAILED));
}

}  // namespace content
