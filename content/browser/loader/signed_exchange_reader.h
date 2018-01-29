// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_LOADER_SIGNED_EXCHANGE_READER_H_
#define CONTENT_BROWSER_LOADER_SIGNED_EXCHANGE_READER_H_

#include "net/base/completion_callback.h"
#include "net/base/io_buffer.h"

namespace content {

// A common reader interface.
class SignedExchangeReader {
 public:
  constexpr static size_t kDefaultBufferSize = 65536;

  virtual ~SignedExchangeReader() {}
  virtual int Read(net::IOBuffer* buffer,
                   int buf_size,
                   const net::CompletionCallback& callback) = 0;
};

}  // namespace content

#endif  // CONTENT_BROWSER_LOADER_SIGNED_EXCHANGE_READER_H_
