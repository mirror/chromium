// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/signed_exchange_handler.h"

#include "base/json/json_reader.h"
#include "base/values.h"
#include "mojo/public/cpp/system/string_data_pipe_producer.h"
#include "net/http/http_response_headers.h"
#include "services/network/public/cpp/resource_response.h"

namespace content {
namespace {

constexpr size_t kPipeSizeForSignedResponseBody = 65536;

std::tuple<GURL, std::string> ParseHTTPExchage(const std::string& data) {
  GURL url;
  std::string payload;

  std::string error_message;
  int error_code = 0;
  std::unique_ptr<base::Value> root = base::JSONReader::ReadAndReturnError(
      data, base::JSON_PARSE_RFC, &error_code, &error_message);
  if (!root || !root->is_list() || root->GetList().size() != 7)
    return std::tie(GURL::EmptyGURL(), payload);
  const base::Value::ListStorage& list = root->GetList();
  if (!list[0].is_string() || list[0].GetString() != "htxg" ||
      !list[1].is_string() || list[1].GetString() != "request" ||
      !list[2].is_dict() || !list[3].is_string() ||
      list[3].GetString() != "response" || !list[4].is_dict() ||
      !list[5].is_string() || list[5].GetString() != "payload" ||
      !list[6].is_string()) {
    return std::tie(GURL::EmptyGURL(), payload);
  }
  const base::Value* url_value = list[2].FindKey(":url");
  if (!url_value || !url_value->is_string())
    return std::tie(GURL::EmptyGURL(), payload);
  url = GURL(url_value->GetString());
  if (!url.is_valid() || !url.SchemeIs(url::kHttpsScheme))
    return std::tie(GURL::EmptyGURL(), payload);
  payload = list[6].GetString();
  return std::tie(url, payload);
}

}  // namespace

SignedExchangeHandler::SignedExchangeHandler(
    mojo::ScopedDataPipeConsumerHandle body)
    : body_(std::move(body)), weak_factory_(this) {}

SignedExchangeHandler::~SignedExchangeHandler() {}

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
  pending_data_.append(static_cast<const char*>(data), num_bytes);
}

void SignedExchangeHandler::OnDataComplete() {
  if (!found_callback_)
    return;

  GURL new_request_url;
  std::string payload;
  std::tie(new_request_url, payload) = ParseHTTPExchage(pending_data_);
  pending_data_.clear();

  // TODO(horo): Get more headers from |pending_data_|.
  scoped_refptr<net::HttpResponseHeaders> headers(
      new net::HttpResponseHeaders("HTTP/1.1 200 OK"));
  network::ResourceResponseHead resource_response;
  resource_response.headers = headers;
  resource_response.mime_type = "text/html";

  mojo::DataPipe pipe(kPipeSizeForSignedResponseBody);
  // TODO(horo): Write Error handling. This could fail.
  DCHECK(pipe.consumer_handle.is_valid());
  mojo::ScopedDataPipeConsumerHandle response_body =
      std::move(pipe.consumer_handle);
  std::move(found_callback_)
      .Run(new_request_url, resource_response, std::move(response_body));

  data_producer_ = std::make_unique<mojo::StringDataPipeProducer>(
      std::move(pipe.producer_handle));
  data_producer_->Write(payload,
                        base::BindOnce(&SignedExchangeHandler::OnDataWritten,
                                       base::Unretained(this)));
}

void SignedExchangeHandler::OnDataWritten(MojoResult result) {
  data_producer_.reset();
  std::move(finish_callback_).Run(0);
}

}  // namespace content
