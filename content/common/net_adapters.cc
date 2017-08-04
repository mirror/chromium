// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/net_adapters.h"

#include "net/base/net_errors.h"

namespace content {

namespace {
const uint32_t kMaxBufSize = 64 * 1024;
}

NetToMojoPendingBuffer::NetToMojoPendingBuffer(
    mojo::ScopedDataPipeProducerHandle handle,
    void* buffer)
    : handle_(std::move(handle)), buffer_(buffer) {}

NetToMojoPendingBuffer::~NetToMojoPendingBuffer() {
  if (handle_.is_valid())
    EndWriteDataRaw(handle_.get(), 0);
}

MojoResult NetToMojoPendingBuffer::BeginWrite(
    mojo::ScopedDataPipeProducerHandle* handle,
    scoped_refptr<NetToMojoPendingBuffer>* pending,
    uint32_t* num_bytes) {
  void* buf;
  *num_bytes = 0;
  MojoResult result = BeginWriteDataRaw(handle->get(), &buf, num_bytes,
                                        MOJO_WRITE_DATA_FLAG_NONE);
  if (result == MOJO_RESULT_OK) {
    if (*num_bytes > kMaxBufSize)
      *num_bytes = kMaxBufSize;
    *pending = new NetToMojoPendingBuffer(std::move(*handle), buf);
  }
  return result;
}

mojo::ScopedDataPipeProducerHandle NetToMojoPendingBuffer::Complete(
    uint32_t num_bytes) {
  EndWriteDataRaw(handle_.get(), num_bytes);
  buffer_ = NULL;
  return std::move(handle_);
}

NetToMojoIOBuffer::NetToMojoIOBuffer(NetToMojoPendingBuffer* pending_buffer,
                                     int offset)
    : net::WrappedIOBuffer(pending_buffer->buffer() + offset),
      pending_buffer_(pending_buffer) {}

NetToMojoIOBuffer::~NetToMojoIOBuffer() {}

MojoToNetPendingBuffer::MojoToNetPendingBuffer(
    mojo::ScopedDataPipeConsumerHandle handle,
    const void* buffer)
    : handle_(std::move(handle)), buffer_(buffer) {}

MojoToNetPendingBuffer::~MojoToNetPendingBuffer() {}

// static
MojoResult MojoToNetPendingBuffer::BeginRead(
    mojo::ScopedDataPipeConsumerHandle* handle,
    scoped_refptr<MojoToNetPendingBuffer>* pending,
    uint32_t* num_bytes) {
  const void* buffer = NULL;
  *num_bytes = 0;
  MojoResult result = BeginReadDataRaw(handle->get(), &buffer, num_bytes,
                                       MOJO_READ_DATA_FLAG_NONE);
  if (result == MOJO_RESULT_OK)
    *pending = new MojoToNetPendingBuffer(std::move(*handle), buffer);
  return result;
}

mojo::ScopedDataPipeConsumerHandle MojoToNetPendingBuffer::Complete(
    uint32_t num_bytes) {
  EndReadDataRaw(handle_.get(), num_bytes);
  buffer_ = NULL;
  return std::move(handle_);
}

MojoToNetIOBuffer::MojoToNetIOBuffer(MojoToNetPendingBuffer* pending_buffer)
    : net::WrappedIOBuffer(pending_buffer->buffer()),
      pending_buffer_(pending_buffer) {}

MojoToNetIOBuffer::~MojoToNetIOBuffer() {}

}  // namespace content
