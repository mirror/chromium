// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/signed_exchange_handler.h"

#include "base/feature_list.h"
#include "content/public/common/content_features.h"
#include "mojo/public/cpp/system/string_data_pipe_producer.h"
#include "net/http/http_response_headers.h"
#include "services/network/public/cpp/resource_response.h"
#include "services/network/public/cpp/url_loader_completion_status.h"

namespace content {

SignedExchangeHandler::SignedExchangeHandler(
    std::unique_ptr<SignedExchangeReader> body,
    ExchangeHeadersCallback headers_callback)
    : body_(std::move(body)), headers_callback_(std::move(headers_callback)) {
  DCHECK(base::FeatureList::IsEnabled(features::kSignedHTTPExchange));
  // Start reading the first chunk.
  int result = ReadData();
  if (result != net::ERR_IO_PENDING)
    MaybeRunHeadersCallback();
}

SignedExchangeHandler::~SignedExchangeHandler() = default;

int SignedExchangeHandler::Read(net::IOBuffer* buf,
                                int buf_size,
                                const net::CompletionCallback& callback) {
  DCHECK(!pending_callback_);

  output_buf_ = buf;
  output_buf_size_ = buf_size;

  if (drainable_input_buf_)
    return ConsumeData();

  int result = ReadData();
  if (result <= 0) {
    if (result == net::ERR_IO_PENDING)
      pending_callback_ = callback;
    return result;
  }
  return ConsumeData();
}

int SignedExchangeHandler::ReadData() {
  if (body_reached_end_)
    return last_result_;

  DCHECK(!input_buf_);
  input_buf_ = base::MakeRefCounted<net::IOBuffer>(kBufferSize);
  int result = body_->Read(input_buf_.get(), kBufferSize,
                           base::BindRepeating(&SignedExchangeHandler::DidRead,
                                               base::Unretained(this)));
  if (result == net::ERR_IO_PENDING)
    return result;
  if (result <= 0) {
    body_reached_end_ = true;
    last_result_ = result;
    return result;
  }

  MakeDrainableBuffer(result);
  return result;
}

void SignedExchangeHandler::DidRead(int result) {
  DCHECK(input_buf_);
  DCHECK(!body_reached_end_);

  if (result <= 0) {
    body_reached_end_ = true;
    last_result_ = net::OK;
    if (pending_callback_)
      std::move(pending_callback_).Run(result);
    return;
  }

  MakeDrainableBuffer(result);

  if (MaybeRunHeadersCallback())
    return;
  if (!pending_callback_)
    return;

  DCHECK(output_buf_);
  int written = ConsumeData();
  DCHECK(written >= 0);
  std::move(pending_callback_).Run(written);
  output_buf_ = nullptr;
  output_buf_size_ = 0;
}

bool SignedExchangeHandler::MaybeRunHeadersCallback() {
  if (!headers_callback_)
    return false;
  DCHECK(!pending_callback_);

  // If this was the first read, fire the headers callback now.
  // TODO(https://crbug.com/80374): This is just for testing, we should
  // implement the CBOR parsing here.
  FillMockExchangeHeaders();
  std::move(headers_callback_)
      .Run(request_url_, request_method_, response_head_, ssl_info_);

  // TODO(https://crbug.com/80374) Consume the bytes size that were
  // necessary to read out the headers.
  // drainable_input_buf_->DidConsume();

  return true;
}

void SignedExchangeHandler::MakeDrainableBuffer(int result) {
  DCHECK(input_buf_);
  DCHECK(!drainable_input_buf_);
  drainable_input_buf_ =
      base::MakeRefCounted<net::DrainableIOBuffer>(input_buf_.get(), result);
  // |input_buf_| is now owned by |drainable_input_buf_|.
  input_buf_ = nullptr;
}

int SignedExchangeHandler::ConsumeData() {
  DCHECK(output_buf_);
  DCHECK(output_buf_size_);
  DCHECK(drainable_input_buf_);

  // For now we just copy the data.
  // TODO(https://crbug.com/80374): Get the payload body by parsing CBOR
  // format. Also if we don't have enough data to process input, we'll
  // need to do a loop calling body_->Read().
  int bytes_to_consume =
      std::min(drainable_input_buf_->BytesRemaining(), output_buf_size_);
  memcpy(output_buf_->data(), drainable_input_buf_->data(),
         base::checked_cast<size_t>(bytes_to_consume));
  drainable_input_buf_->DidConsume(bytes_to_consume);
  if (drainable_input_buf_->BytesRemaining() == 0)
    drainable_input_buf_ = nullptr;

  return bytes_to_consume;
}

void SignedExchangeHandler::FillMockExchangeHeaders() {
  // TODO(https://crbug.com/80374): Get the request url by parsing CBOR format.
  request_url_ = GURL("https://example.com/test.html");
  // TODO(https://crbug.com/80374): Get the request method by parsing CBOR
  // format.
  request_method_ = "GET";
  // TODO(https://crbug.com/80374): Get more headers by parsing CBOR.
  scoped_refptr<net::HttpResponseHeaders> headers(
      new net::HttpResponseHeaders("HTTP/1.1 200 OK"));
  response_head_.headers = headers;
  response_head_.mime_type = "text/html";
}

}  // namespace content
