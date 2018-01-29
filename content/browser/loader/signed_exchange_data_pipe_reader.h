// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_LOADER_SIGNED_EXCHANGE_DATA_PIPE_READER_H_
#define CONTENT_BROWSER_LOADER_SIGNED_EXCHANGE_DATA_PIPE_READER_H_

#include "content/browser/loader/signed_exchange_reader.h"
#include "mojo/public/cpp/system/data_pipe.h"
#include "mojo/public/cpp/system/simple_watcher.h"

namespace content {

class SignedExchangeDataPipeReader final : public SignedExchangeReader {
 public:
  explicit SignedExchangeDataPipeReader(
      mojo::ScopedDataPipeConsumerHandle body);
  ~SignedExchangeDataPipeReader() override;

  int Read(net::IOBuffer* buf,
           int buf_size,
           const net::CompletionCallback& callback) override;

 private:
  void OnReadable(MojoResult result);
  void FinishReading();

  mojo::ScopedDataPipeConsumerHandle body_;
  mojo::SimpleWatcher handle_watcher_;

  scoped_refptr<net::IOBuffer> output_buf_;
  int output_buf_size_ = 0;
  net::CompletionCallback pending_callback_;

  DISALLOW_COPY_AND_ASSIGN(SignedExchangeDataPipeReader);
};

}  // namespace content

#endif  // CONTENT_BROWSER_LOADER_SIGNED_EXCHANGE_DATA_PIPE_READER_H_
