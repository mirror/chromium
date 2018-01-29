// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_LOADER_SIGNED_EXCHANGE_DATA_PIPE_READER_H_
#define CONTENT_BROWSER_LOADER_SIGNED_EXCHANGE_DATA_PIPE_READER_H_

#include "content/browser/loader/signed_exchange_reader.h"

class SignedExchangeDataPipeReader final
    : public SignedExchangeReader,
      public mojo::common::DataPipeDrainer::Client {
 public:
  explicit SignedExchangeDataPipeReader(
      mojo::ScopedDataPipeConsumerHandle body);
  SignedExchangeDataPipeReader() override;

  int Read(net::IOBuffer* buf,
           int buf_size,
           const net::CompletionCallback& callback) override;

 private:
  // mojo::Common::DataPipeDrainer::Client
  void OnDataAvailable(const void* data, size_t num_bytes) override;
  void OnDataComplete() override;

  mojo::ScopedDataPipeConsumerHandle body_;
  std::unique_ptr<mojo::common::DataPipeDrainer> drainer_;

  DISALLOW_COPY_AND_ASSIGN(SignedExchangeHandler);
};

#endif  // CONTENT_BROWSER_LOADER_SIGNED_EXCHANGE_DATA_PIPE_READER_H_
