// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/net/chunked_data_stream_uploader.h"

#include "net/base/io_buffer.h"
#include "net/base/net_errors.h"

namespace net {

ChunkedDataStreamUploader::ChunkedDataStreamUploader(Delegate* delegate)
    : UploadDataStream(true, 0),
      delegate_(delegate),
      pending_read_buf_(nullptr),
      pending_read_buf_len_(0),
      pending_stream_data_(false),
      pending_internal_read_(false),
      final_chunk_(false),
      at_front_of_stream_(true),
      weak_factory_(this) {}

ChunkedDataStreamUploader::~ChunkedDataStreamUploader() {}

int ChunkedDataStreamUploader::InitInternal(const NetLogWithSource& net_log) {
  if (at_front_of_stream_)
    return net::OK;
  else
    return net::ERR_FAILED;
}

int ChunkedDataStreamUploader::ReadInternal(net::IOBuffer* buf, int buf_len) {
  DCHECK(buf);
  DCHECK_GT(buf_len, 0);

  pending_read_buf_ = buf;
  pending_read_buf_len_ = buf_len;

  // Read the stream if input data comes first.
  if (pending_stream_data_) {
    return Upload();
  } else {
    pending_internal_read_ = true;
    return net::ERR_IO_PENDING;
  }
}

void ChunkedDataStreamUploader::ResetInternal() {
  pending_read_buf_ = nullptr;
  pending_read_buf_len_ = 0;
  pending_internal_read_ = false;
  // Internal reset will not affect the external stream data state.
  final_chunk_ = false;
}

void ChunkedDataStreamUploader::UploadWhenReady(bool final_chunk) {
  final_chunk_ = final_chunk;

  // Put the data if internal read comes first.
  if (pending_internal_read_) {
    Upload();
  } else {
    pending_stream_data_ = true;
  }
}

int ChunkedDataStreamUploader::Upload() {
  DCHECK(pending_read_buf_);
  DCHECK_NE(pending_stream_data_, pending_internal_read_);

  at_front_of_stream_ = false;
  pending_stream_data_ = false;
  int bytes_read =
      delegate_->OnRead(pending_read_buf_->data(), pending_read_buf_len_);

  // iOS NSInputstream can read 0 byte when NSStreamEventHasBytesAvailable
  // triggered, so ignore this piece and let this internal read remains pending.
  if (bytes_read == 0 && !final_chunk_) {
    pending_internal_read_ = true;
    return net::ERR_IO_PENDING;
  }

  if (final_chunk_) {
    SetIsFinalChunk();
    delegate_->OnFinishUpload();
  }

  pending_read_buf_ = nullptr;
  pending_read_buf_len_ = 0;

  // When has Read() pending, should call OnReadCompleted to notify read
  // completed.
  if (pending_internal_read_) {
    pending_internal_read_ = false;
    OnReadCompleted(bytes_read);
  }
  return bytes_read;
}

}  // namespace net
