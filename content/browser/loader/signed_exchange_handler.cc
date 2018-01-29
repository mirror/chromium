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

namespace {

scoped_refptr<net::DrainableIOBuffer> MakeDrainableIOBufferWithIOBuffer() {
  return base::MakeRefCounted<net::DrainableIOBuffer>(
      new net::IOBuffer(SignedExchangeReader::kDefaultBufferSize),
      SignedExchangeReader::kDefaultBufferSize);
}

};  // namespace

SignedExchangeHandler::SignedExchangeHandler(
    std::unique_ptr<BodyReader> body,
    ExchangeHeadersCallback headers_callback)
    : body_(std::move(body)),
      headers_callback_(std::move(headers_callback)),
      input_buf_(base::MakeRefCounted<net::IOBuffer>(kBufferSize)) {
  DCHECK(base::FeatureList::IsEnabled(features::kSignedHTTPExchange));
  // Start reading the first chunk.
  body_->Read(
      input_buf_, input_buf_->BytesRemaining(),
      base::Bind(&SignedExchangeHandler::DidRead, base::Unretained(this)));
}

SignedExchangeHandler::~SignedExchangeHandler() = default;

int SignedExchangeHandler::Read(net::IOBuffer* buf,
                                int buf_size,
                                const net::CompletionCallback& callback) {
  DCHECK(!body_reached_end_);
  DCHECK(!pending_callback_);

  output_buf_ = buf;
  output_buf_size_ = buf_size;

  if (drainable_input_buf_)
    return ProcessData();

  DCHECK(!input_buf_);
  input_buf_ = base::MakeRefCounted<net::IOBuffer>(kBufferSize);
  int result = body_->Read(
      input_buf_, input_buf_->BytesRemaining(),
      base::Bind(&SignedExchangeHandler::DidRead, base::Unretained(this)));
  if (result == net::ERR_IO_PENDING) {
    pending_callback_ = callback;
    return;
  }
  if (result <= 0) {
    body_reached_end_ = true;
    return result;
  }

  MakeDrainableBuffer(result);
  return ProcessData();
}

void SignedExchangeHandler::DidRead(int result) {
  DCHECK(input_buf_);
  DCHECK(!body_reached_end_);

  MakeDrainableBuffer(result);

  if (MaybeRunHeadersCallback())
    return;

  DCHECK(pending_callback_);
  DCHECK(output_buf_);
  int written = ProcessData();
  // TODO(https://crbug.com/80374): We need to run a loop to
  // call body_->Read() if ProcessData() needs more data.
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
  FillMockExchangeHandler();
  std::move(headers_callback_)
      .Run(url_, request_method_, response_head_, ssl_info_);

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

int SignedExchangeHandler::ProcessData() {
  DCHECK(output_buf_);
  DCHECK(output_buf_size_);
  DCHECK(drainable_input_buf_);

  // For now we just copy the data.
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
  // TODO(https://crbug.com/80374): Get the payload by parsing CBOR format.
  std::string payload = original_body_string_;
  original_body_string_.clear();

  // TODO(https://crbug.com/80374): Get more headers by parsing CBOR.
  scoped_refptr<net::HttpResponseHeaders> headers(
      new net::HttpResponseHeaders("HTTP/1.1 200 OK"));
  response_head_.headers = headers;
  response_head_.mime_type = "text/html";
}

}  // namespace content
