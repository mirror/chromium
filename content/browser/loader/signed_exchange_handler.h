// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_LOADER_SIGNED_EXCHANGE_HANDLER_H_
#define CONTENT_BROWSER_LOADER_SIGNED_EXCHANGE_HANDLER_H_

#include <string>

#include "base/callback.h"
#include "base/optional.h"
#include "mojo/common/data_pipe_drainer.h"
#include "mojo/public/cpp/system/data_pipe.h"
#include "net/base/completion_callback.h"
#include "net/filter/source_stream.h"
#include "net/ssl/ssl_info.h"
#include "services/network/public/cpp/resource_response.h"
#include "url/gurl.h"

namespace net {
class DrainableIOBuffer;
}

namespace content {

// IMPORTANT: Currenly SignedExchangeHandler doesn't implement any CBOR parsing
// logic nor verifying logic. It just behaves as if the passed body is a signed
// HTTP exchange which contains a request to "https://example.com/test.html" and
// a response with a payload which is equal to the original body.
// TODO(https://crbug.com/803774): Implement CBOR parsing logic and verifying
// logic.
class SignedExchangeHandler final : public net::SourceStream {
 public:
  // TODO(https://crbug.com/803774): Add verification status here.
  using ExchangeHeadersCallback =
      base::OnceCallback<void(const GURL& request_url,
                              const std::string& request_method,
                              const network::ResourceResponseHead&,
                              base::Optional<net::SSLInfo>)>;
  SignedExchangeHandler(std::unique_ptr<net::SourceStream> body,
                        ExchangeHeadersCallback headers_callback);
  ~SignedExchangeHandler() override;

  // net::SourceStream:
  int Read(net::IOBuffer* buf,
           int buf_size,
           const net::CompletionCallback& callback) override;
  std::string Description() const override;

 private:
  enum class ReadState {
    kNone,
    // Reading data from |body_| into |input_buf_|.
    kReadData,
    // Reading data is completed, creates a |drainable_input_buf_|.
    kReadDataComplete,
    // Drain (process) data in the |input_buf_| and fill |output_buf_|.
    kDrainData,
  };

  int DoLoop(int result);
  int DoReadData();
  int DoReadDataComplete(int result);
  int DoDrainData();

  void DidReadAsync(int result);

  // Maybe run |headers_callback_|.
  bool MaybeRunHeadersCallback();

  // TODO(https://crbug.com/803774): Remove this.
  void FillMockExchangeHeaders();

  ReadState next_state_ = ReadState::kNone;

  // Signed exchange contents.
  GURL request_url_;
  std::string request_method_;
  network::ResourceResponseHead response_head_;
  base::Optional<net::SSLInfo> ssl_info_;

  std::unique_ptr<net::SourceStream> body_;
  ExchangeHeadersCallback headers_callback_;

  scoped_refptr<net::IOBuffer> input_buf_;
  scoped_refptr<net::DrainableIOBuffer> drainable_input_buf_;

  scoped_refptr<net::IOBuffer> output_buf_;
  int output_buf_size_ = 0;
  net::CompletionCallback pending_callback_;

  bool body_reached_end_ = false;

  DISALLOW_COPY_AND_ASSIGN(SignedExchangeHandler);
};

}  // namespace content

#endif  // CONTENT_BROWSER_LOADER_SIGNED_EXCHANGE_HANDLER_H_
