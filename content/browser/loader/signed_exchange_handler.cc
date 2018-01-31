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

SignedExchangeHandler::SignedExchangeHandler(
    std::unique_ptr<net::SourceStream> upstream,
    ExchangeHeadersCallback headers_callback)
    : net::FilterSourceStream(net::SourceStream::TYPE_NONE,
                              std::move(upstream)),
      headers_callback_(std::move(headers_callback)) {
  DCHECK(base::FeatureList::IsEnabled(features::kSignedHTTPExchange));

  // Triggering the first read for header parsing.
  header_out_buf_ = base::MakeRefCounted<net::IOBufferWithSize>(1);
  DoHeaderLoop();
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
  // in a header reading mode.
  if (read_state_ == kHeaders)
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
                                    base::Unretained(this)));
  if (rv != net::ERR_IO_PENDING)
    DidReadForHeaders(rv);
}

void SignedExchangeHandler::DidReadForHeaders(int result) {
  if (MaybeRunHeadersCallback() || result < 0) {
    read_state_ = kBody;
    return;
  }
  DCHECK_EQ(0, result);
  DoHeaderLoop();
}

bool SignedExchangeHandler::MaybeRunHeadersCallback() {
  if (!headers_callback_)
    return false;

  // If this was the first read, fire the headers callback now.
  // TODO(https://crbug.com/803774): This is just for testing, we should
  // implement the CBOR parsing here.
  FillMockExchangeHeaders();
  std::move(headers_callback_)
      .Run(request_url_, request_method_, response_head_, ssl_info_);

  // TODO(https://crbug.com/803774) Consume the bytes size that were
  // necessary to read out the headers.

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
