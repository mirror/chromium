// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/signed_exchange_handler.h"

#include "base/feature_list.h"
#include "components/cbor/cbor_reader.h"
#include "content/public/common/content_features.h"
#include "mojo/public/cpp/system/string_data_pipe_producer.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_util.h"
#include "services/network/public/cpp/resource_response.h"
#include "services/network/public/cpp/url_loader_completion_status.h"

namespace content {
namespace {

constexpr size_t kPipeSizeForSignedResponseBody = 65536;
constexpr char kUrlKey[] = ":url";
constexpr char kMethodKey[] = ":method";
constexpr char kStatusKey[] = ":status";

}  // namespace

SignedExchangeHandler::SignedExchangeHandler(
    mojo::ScopedDataPipeConsumerHandle body)
    : body_(std::move(body)) {
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

  cbor::CBORReader::DecoderError error;
  base::Optional<cbor::CBORValue> root = cbor::CBORReader::Read(
      base::span<const uint8_t>(
          reinterpret_cast<const uint8_t*>(original_body_string_.data()),
          original_body_string_.size()),
      &error, cbor::CBORReader::kCBORMaxDepth,
      cbor::CBORReader::Mode::CANONICAL);
  std::string payload;
  CHECK(root) << "CBOR parsing failed: "
              << cbor::CBORReader::ErrorCodeToString(error);
  payload = "dummy content";
  original_body_string_.clear();

  const auto& root_array = root->GetArray();
  CHECK_EQ("htxg", root_array[0].GetString());
  CHECK_EQ("request", root_array[1].GetString());
  const auto& request_map = root_array[2].GetMap();
  CHECK_EQ("response", root_array[3].GetString());
  const auto& response_map = root_array[4].GetMap();
  (void)response_map;
  CHECK_EQ("payload", root_array[5].GetString());
  const auto& payload_bytes = root_array[6].GetBytestring();
  (void)payload_bytes;

  const cbor::CBORValue url_key_value =
      cbor::CBORValue::BytestringFromString(kUrlKey);
  GURL request_url =
      GURL(request_map.find(url_key_value)->second.GetBytestringAsString());

  const cbor::CBORValue method_key_value =
      cbor::CBORValue::BytestringFromString(kMethodKey);
  base::StringPiece request_method =
      request_map.find(url_key_value)->second.GetBytestringAsString();

  const cbor::CBORValue status_key_value =
      cbor::CBORValue::BytestringFromString(kStatusKey);
  base::StringPiece status_code_str =
      response_map.find(status_key_value)->second.GetBytestringAsString();
  std::string fake_header_str("HTTP/1.1 ");
  status_code_str.AppendToString(&fake_header_str);
  fake_header_str.append(" OK\r\n");
  for (const auto& it : response_map) {
    base::StringPiece name = it.first.GetBytestringAsString();
    base::StringPiece value = it.second.GetBytestringAsString();
    if (name == kMethodKey)
      continue;

    name.AppendToString(&fake_header_str);
    fake_header_str.append(": ");
    value.AppendToString(&fake_header_str);
    fake_header_str.append("\r\n");
  }
  fake_header_str.append("\r\n");
  network::ResourceResponseHead resource_response;
  resource_response.headers = base::MakeRefCounted<net::HttpResponseHeaders>(
      net::HttpUtil::AssembleRawHeaders(fake_header_str.c_str(),
                                        fake_header_str.size()));
  // TODO(803774): |mime_type| should be derived from "Content-Type" header.
  resource_response.mime_type = "text/html";

  // TODO(https://crbug.com/80374): Get the certificate by parsing CBOR and
  // verify.
  base::Optional<net::SSLInfo> ssl_info;

  mojo::DataPipe pipe(kPipeSizeForSignedResponseBody);
  // TODO(https://crbug.com/80374): Write the error handling code. This could
  // fail.
  DCHECK(pipe.consumer_handle.is_valid());
  mojo::ScopedDataPipeConsumerHandle response_body =
      std::move(pipe.consumer_handle);
  std::move(found_callback_)
      .Run(request_url, request_method.as_string(), resource_response,
           std::move(ssl_info), std::move(response_body));

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
