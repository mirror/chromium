// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/signed_exchange_handler.h"

#include "base/feature_list.h"
#include "content/public/common/content_features.h"
#include "mojo/public/cpp/system/string_data_pipe_producer.h"
#include "net/base/io_buffer.h"
#include "net/http/http_response_headers.h"
#include "services/network/public/cpp/resource_response.h"
#include "services/network/public/cpp/url_loader_completion_status.h"

namespace content {

namespace {
constexpr static int kBufferSize = 64 * 1024;
}  // namespace

SignedExchangeHandler::SignedExchangeHandler(
    std::unique_ptr<net::SourceStream> body,
    ExchangeHeadersCallback headers_callback)
    : net::SourceStream(net::SourceStream::TYPE_NONE),
      body_(std::move(body)),
      headers_callback_(std::move(headers_callback)),
      input_buf_(base::MakeRefCounted<net::IOBuffer>(kBufferSize)) {
  DCHECK(base::FeatureList::IsEnabled(features::kSignedHTTPExchange));

  // Start reading the first chunk.
  next_state_ = ReadState::kReadData;
  DoLoop(net::OK);
}

SignedExchangeHandler::~SignedExchangeHandler() = default;

std::string SignedExchangeHandler::Description() const {
  return "SignedExchangeHandler";
}

int SignedExchangeHandler::Read(net::IOBuffer* buf,
                                int buf_size,
                                const net::CompletionCallback& callback) {
  DCHECK(!pending_callback_);

  if (!drainable_input_buf_) {
    next_state_ = ReadState::kReadData;
  } else {
    DCHECK(drainable_input_buf_);
    next_state_ = ReadState::kDrainData;
  }

  output_buf_ = buf;
  output_buf_size_ = buf_size;

  int rv = DoLoop(net::OK);
  if (rv == net::ERR_IO_PENDING)
    pending_callback_ = callback;
  return rv;
}

int SignedExchangeHandler::DoLoop(int result) {
  DCHECK_NE(ReadState::kNone, next_state_);

  int rv = result;
  do {
    ReadState state = next_state_;
    next_state_ = ReadState::kNone;
    switch (state) {
      case ReadState::kReadData:
        rv = DoReadData();
        break;
      case ReadState::kReadDataComplete:
        rv = DoReadDataComplete(rv);
        break;
      case ReadState::kDrainData:
        DCHECK_LE(0, rv);
        rv = DoDrainData();
        break;
      default:
        NOTREACHED() << "bad state: " << static_cast<int>(state);
        rv = net::ERR_UNEXPECTED;
        break;
    }
  } while (rv != net::ERR_IO_PENDING && next_state_ != ReadState::kNone);
  return rv;
}

int SignedExchangeHandler::DoReadData() {
  DCHECK(!drainable_input_buf_);
  DCHECK(!body_reached_end_);

  next_state_ = ReadState::kReadDataComplete;

  return body_->Read(input_buf_.get(), kBufferSize,
                     base::BindRepeating(&SignedExchangeHandler::DidReadAsync,
                                         base::Unretained(this)));
}

int SignedExchangeHandler::DoReadDataComplete(int result) {
  DCHECK_NE(net::ERR_IO_PENDING, result);
  DCHECK(input_buf_);
  DCHECK(!drainable_input_buf_);

  if (result <= 0) {
    body_reached_end_ = true;
    DCHECK_EQ(ReadState::kNone, next_state_);
    return result;
  }
  drainable_input_buf_ =
      base::MakeRefCounted<net::DrainableIOBuffer>(input_buf_.get(), result);
  next_state_ = ReadState::kDrainData;

  return result;
}

int SignedExchangeHandler::DoDrainData() {
  DCHECK(drainable_input_buf_);

  if (MaybeRunHeadersCallback()) {
    DCHECK_EQ(next_state_, ReadState::kNone);
    return 0;
  }

  DCHECK(output_buf_);
  DCHECK(output_buf_size_);

  // TODO(https://crbug.com/803774): Get the payload body by parsing CBOR
  // format.  For now we just copy the data.
  int bytes_to_consume =
      std::min(drainable_input_buf_->BytesRemaining(), output_buf_size_);
  memcpy(output_buf_->data(), drainable_input_buf_->data(),
         base::checked_cast<size_t>(bytes_to_consume));
  drainable_input_buf_->DidConsume(bytes_to_consume);

  if (drainable_input_buf_->BytesRemaining() == 0) {
    drainable_input_buf_ = nullptr;
    if (body_reached_end_ /* need more data? */) {
      // Loop one more time to see if we really have no data
      // (to match the behavior with FilterSourceStream).
      next_state_ = ReadState::kReadData;
    }
  }

  return bytes_to_consume;
}

void SignedExchangeHandler::DidReadAsync(int result) {
  DCHECK_EQ(ReadState::kReadDataComplete, next_state_);

  int rv = DoLoop(result);
  if (rv == net::ERR_IO_PENDING)
    return;

  if (!pending_callback_)
    return;

  output_buf_ = nullptr;
  output_buf_size_ = 0;

  std::move(pending_callback_).Run(rv);
}

bool SignedExchangeHandler::MaybeRunHeadersCallback() {
  if (!headers_callback_)
    return false;
  DCHECK(!pending_callback_);

  // If this was the first read, fire the headers callback now.
  // TODO(https://crbug.com/803774): This is just for testing, we should
  // implement the CBOR parsing here.
  FillMockExchangeHeaders();
  std::move(headers_callback_)
      .Run(request_url_, request_method_, response_head_, ssl_info_);

  // TODO(https://crbug.com/803774) Consume the bytes size that were
  // necessary to read out the headers.
  // drainable_input_buf_->DidConsume();

  return true;
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
