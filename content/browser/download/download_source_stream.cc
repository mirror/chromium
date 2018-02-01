// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "content/browser/download/download_source_stream.h"

#include <string>
#include <utility>

#include "content/browser/byte_stream.h"

namespace content {

namespace {

// Because DownloadSaveInfo::kLengthFullContent is 0, we should avoid using
// 0 for length if we found that a stream can no longer write any data.
const int kNoBytesToWrite = -1;

// Data length to read from data pipe.
const int kBytesToRead = 4096;

}  // namespace

DownloadSourceStream::DownloadSourceStream(
    int64_t offset,
    int64_t length,
    std::unique_ptr<DownloadManager::InputStream> stream)
    : offset_(offset),
      length_(length),
      bytes_written_(0),
      finished_(false),
      index_(0u),
      stream_reader_(std::move(stream->stream_reader_)),
      completion_status_(DOWNLOAD_INTERRUPT_REASON_NONE),
      is_response_completed_(false),
      stream_handle_(std::move(stream->stream_handle_)) {}

DownloadSourceStream::~DownloadSourceStream() = default;

void DownloadSourceStream::Initialize() {
  if (stream_handle_.is_null())
    return;
  binding_ = base::MakeUnique<mojo::Binding<mojom::DownloadStreamClient>>(
      this, std::move(stream_handle_->client_request));
  binding_->set_connection_error_handler(base::BindOnce(
      &DownloadSourceStream::OnStreamCompleted, base::Unretained(this),
      mojom::NetworkRequestStatus::USER_CANCELED));
  handle_watcher_ = base::MakeUnique<mojo::SimpleWatcher>(
      FROM_HERE, mojo::SimpleWatcher::ArmingPolicy::AUTOMATIC);
}

void DownloadSourceStream::OnStreamCompleted(
    mojom::NetworkRequestStatus status) {
  // This can be called before or after data pipe is completely drained. So we
  // need to pass the |completion_status_| to DownloadFileImpl if the data pipe
  // is already drained.
  OnResponseCompleted(ConvertMojoNetworkRequestStatusToInterruptReason(status));
}

void DownloadSourceStream::OnResponseCompleted(DownloadInterruptReason reason) {
  is_response_completed_ = true;
  completion_status_ = reason;
  if (completion_callback_)
    std::move(completion_callback_).Run(this);
}

void DownloadSourceStream::OnWriteBytesToDisk(int64_t bytes_write) {
  bytes_written_ += bytes_write;
}

void DownloadSourceStream::TruncateLengthWithWrittenDataBlock(
    int64_t offset,
    int64_t bytes_written) {
  DCHECK_GT(bytes_written, 0);
  if (length_ == kNoBytesToWrite)
    return;

  if (offset <= offset_) {
    if (offset + bytes_written > offset_) {
      length_ = kNoBytesToWrite;
      finished_ = true;
    }
    return;
  }

  if (length_ == DownloadSaveInfo::kLengthFullContent ||
      length_ > offset - offset_) {
    length_ = offset - offset_;
  }
}

void DownloadSourceStream::RegisterDataReadyCallback(
    const mojo::SimpleWatcher::ReadyCallback& callback) {
  if (handle_watcher_) {
    handle_watcher_->Watch(stream_handle_->stream.get(),
                           MOJO_HANDLE_SIGNAL_READABLE, callback);
  } else if (stream_reader_) {
    stream_reader_->RegisterCallback(
        base::BindRepeating(callback, MOJO_RESULT_OK));
  }
}

void DownloadSourceStream::ClearDataReadyCallback() {
  if (handle_watcher_)
    handle_watcher_->Cancel();
  else if (stream_reader_)
    stream_reader_->RegisterCallback(base::RepeatingClosure());
}

DownloadInterruptReason DownloadSourceStream::GetCompletionStatus() {
  if (stream_reader_)
    return static_cast<DownloadInterruptReason>(stream_reader_->GetStatus());
  return completion_status_;
}

void DownloadSourceStream::RegisterCompletionCallback(
    DownloadSourceStream::CompletionCallback callback) {
  completion_callback_ = std::move(callback);
}

DownloadSourceStream::StreamState DownloadSourceStream::Read(
    scoped_refptr<net::IOBuffer>* data,
    size_t* length) {
  if (handle_watcher_) {
    *length = kBytesToRead;
    *data = new net::IOBuffer(kBytesToRead);
    MojoResult mojo_result = stream_handle_->stream->ReadData(
        (*data)->data(), (uint32_t*)length, MOJO_READ_DATA_FLAG_NONE);
    // TODO(qinmin): figure out when COMPLETE should be returned.
    switch (mojo_result) {
      case MOJO_RESULT_OK:
        return HAS_DATA;
      case MOJO_RESULT_SHOULD_WAIT:
        return EMPTY;
      case MOJO_RESULT_FAILED_PRECONDITION:
        if (is_response_completed_)
          return COMPLETE;
        stream_handle_->stream.reset();
        ClearDataReadyCallback();
        return WAIT_FOR_COMPLETION;
      case MOJO_RESULT_INVALID_ARGUMENT:
      case MOJO_RESULT_OUT_OF_RANGE:
      case MOJO_RESULT_BUSY:
        NOTREACHED();
        return COMPLETE;
    }
  } else if (stream_reader_) {
    ByteStreamReader::StreamState state = stream_reader_->Read(data, length);
    switch (state) {
      case ByteStreamReader::STREAM_EMPTY:
        return EMPTY;
      case ByteStreamReader::STREAM_HAS_DATA:
        return HAS_DATA;
      case ByteStreamReader::STREAM_COMPLETE:
        return COMPLETE;
    }
  }
  return COMPLETE;
}

}  // namespace content