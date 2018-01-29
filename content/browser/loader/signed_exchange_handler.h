// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_LOADER_SIGNED_EXCHANGE_HANDLER_H_
#define CONTENT_BROWSER_LOADER_SIGNED_EXCHANGE_HANDLER_H_

#include <string>

#include "base/callback.h"
#include "base/optional.h"
#include "content/browser/loader/signed_exchange_reader.h"
#include "mojo/common/data_pipe_drainer.h"
#include "mojo/public/cpp/system/data_pipe.h"
#include "net/base/completion_callback.h"
#include "net/ssl/ssl_info.h"
#include "services/network/public/cpp/resource_response.h"
#include "url/gurl.h"

namespace content {

// IMPORTANT: Currenly SignedExchangeHandler doesn't implement any CBOR parsing
// logic nor verifying logic. It just behaves as if the passed body is a signed
// HTTP exchange which contains a request to "https://example.com/test.html" and
// a response with a payload which is equal to the original body.
// TODO(https://crbug.com/80374): Implement CBOR parsing logic and verifying
// logic.
class SignedExchangeHandler final : public SignedExchangeReader {
 public:
  constexpr static int kBufferSize = SignedExchangeReader::kDefaultBufferSize;

  // TODO(https://crbug.com/80374): Add verification status here.
  using ExchangeHeadersCallback =
      base::OnceCallback<void(const GURL& request_url,
                              const std::string& request_method,
                              const network::ResourceResponseHead&,
                              base::Optional<net::SSLInfo>)>;

  SignedExchangeHandler(std::unique_ptr<SignedExchangeReader> body,
                        ExchangeHeadersCallback headers_callback);
  ~SignedExchangeHandler() override;

  // SignedExchangeReader:
  int Read(net::IOBuffer* buf,
           int buf_size,
           const net::CompletionCallback& callback) override;

 private:
  int ReadData();

  // Called by |body_|->Read().
  void DidRead(int result);

  // Returns true if this fired the headers callback.
  bool MaybeRunHeadersCallback();

  // Creates an internal |drainable_input_buf_| from |input_buf_|.
  void MakeDrainableBuffer(int result);

  // Consumes |drainable_input_buf_| and write the processed data out to
  // |output_buf_|. Returns the # of bytes written to |output_buf|.
  int ConsumeData();

  // TODO(https://crbug.com/80374): Remove this.
  void FillMockExchangeHeaders();

  // Signed exchange contents.
  GURL request_url_;
  std::string request_method_;
  network::ResourceResponseHead response_head_;
  base::Optional<net::SSLInfo> ssl_info_;

  std::unique_ptr<SignedExchangeReader> body_;
  ExchangeHeadersCallback headers_callback_;

  scoped_refptr<net::IOBuffer> input_buf_;
  scoped_refptr<net::DrainableIOBuffer> drainable_input_buf_;

  scoped_refptr<net::IOBuffer> output_buf_;
  int output_buf_size_ = 0;
  net::CompletionCallback pending_callback_;

  bool body_reached_end_ = false;
  int last_result_ = 0;

  DISALLOW_COPY_AND_ASSIGN(SignedExchangeHandler);
};

}  // namespace content

#endif  // CONTENT_BROWSER_LOADER_SIGNED_EXCHANGE_HANDLER_H_
