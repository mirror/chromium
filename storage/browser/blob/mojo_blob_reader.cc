// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "storage/browser/blob/mojo_blob_reader.h"

#include "net/base/io_buffer.h"
#include "storage/browser/blob/blob_data_handle.h"

namespace storage {

namespace {
const uint32_t kMaxBufSize = 64 * 1024;
}

MojoBlobReader::Delegate::RequestSideData
MojoBlobReader::Delegate::DidCalculateSize(uint64_t, uint64_t) {
  return DONT_REQUEST_SIDE_DATA;
}

class MojoBlobReader::NetToMojoPendingBuffer
    : public base::RefCounted<MojoBlobReader::NetToMojoPendingBuffer> {
 public:
  // Begins a two-phase write to the data pipe.
  //
  // On success, MOJO_RESULT_OK will be returned. The ownership of the given
  // producer handle will be transferred to the new NetToMojoPendingBuffer that
  // will be placed into *pending, and the size of the buffer will be in
  // *num_bytes.
  //
  // On failure or MOJO_RESULT_SHOULD_WAIT, there will be no change to the
  // handle, and *pending and *num_bytes will be unused.
  static MojoResult BeginWrite(mojo::ScopedDataPipeProducerHandle* handle,
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

  // Called to indicate the buffer is done being written to. Passes ownership
  // of the pipe back to the caller.
  mojo::ScopedDataPipeProducerHandle Complete(uint32_t num_bytes) {
    EndWriteDataRaw(handle_.get(), num_bytes);
    buffer_ = NULL;
    return std::move(handle_);
  }

  char* buffer() { return static_cast<char*>(buffer_); }

 private:
  friend class base::RefCounted<NetToMojoPendingBuffer>;
  // Takes ownership of the handle.
  NetToMojoPendingBuffer(mojo::ScopedDataPipeProducerHandle handle,
                         void* buffer)
      : handle_(std::move(handle)), buffer_(buffer) {}
  ~NetToMojoPendingBuffer() {
    if (handle_.is_valid())
      EndWriteDataRaw(handle_.get(), 0);
  }
  mojo::ScopedDataPipeProducerHandle handle_;
  void* buffer_;
  DISALLOW_COPY_AND_ASSIGN(NetToMojoPendingBuffer);
};

class MojoBlobReader::NetToMojoIOBuffer : public net::WrappedIOBuffer {
 public:
  explicit NetToMojoIOBuffer(NetToMojoPendingBuffer* pending_buffer)
      : net::WrappedIOBuffer(pending_buffer->buffer()),
        pending_buffer_(pending_buffer) {}

 private:
  ~NetToMojoIOBuffer() override {}
  scoped_refptr<NetToMojoPendingBuffer> pending_buffer_;
};

// static
void MojoBlobReader::Create(FileSystemContext* file_system_context,
                            const BlobDataHandle* handle,
                            const net::HttpByteRange& range,
                            std::unique_ptr<Delegate> delegate) {
  new MojoBlobReader(file_system_context, handle, range, std::move(delegate));
}

MojoBlobReader::MojoBlobReader(FileSystemContext* file_system_context,
                               const BlobDataHandle* handle,
                               const net::HttpByteRange& range,
                               std::unique_ptr<Delegate> delegate)
    : delegate_(std::move(delegate)),
      byte_range_(range),
      blob_reader_(handle ? handle->CreateReader(file_system_context)
                          : nullptr),
      writable_handle_watcher_(FROM_HERE,
                               mojo::SimpleWatcher::ArmingPolicy::MANUAL),
      peer_closed_handle_watcher_(FROM_HERE,
                                  mojo::SimpleWatcher::ArmingPolicy::MANUAL),
      weak_factory_(this) {
  DCHECK(delegate_);
  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::Bind(&MojoBlobReader::Start, weak_factory_.GetWeakPtr()));
}

MojoBlobReader::~MojoBlobReader() {}

void MojoBlobReader::Start() {
  if (!blob_reader_) {
    NotifyCompleted(net::ERR_FILE_NOT_FOUND);
    return;
  }

  if (blob_reader_->net_error()) {
    NotifyCompleted(blob_reader_->net_error());
    return;
  }

  BlobReader::Status size_status = blob_reader_->CalculateSize(
      base::Bind(&MojoBlobReader::DidCalculateSize, base::Unretained(this)));
  switch (size_status) {
    case BlobReader::Status::NET_ERROR:
      NotifyCompleted(blob_reader_->net_error());
      return;
    case BlobReader::Status::IO_PENDING:
      return;
    case BlobReader::Status::DONE:
      DidCalculateSize(net::OK);
      return;
  }

  NOTREACHED();
}

void MojoBlobReader::NotifyCompleted(int result) {
  if (!notified_completed_) {
    delegate_->OnComplete(static_cast<net::Error>(result),
                          total_written_bytes_);
    notified_completed_ = true;
  }

  bool has_data_pipe = pending_write_.get() || response_body_stream_.is_valid();
  if (!has_data_pipe)
    delete this;
}

void MojoBlobReader::DidCalculateSize(int result) {
  if (result != net::OK) {
    NotifyCompleted(result);
    return;
  }

  // Apply the range requirement.
  if (!byte_range_.ComputeBounds(blob_reader_->total_size())) {
    NotifyCompleted(net::ERR_REQUEST_RANGE_NOT_SATISFIABLE);
    return;
  }

  DCHECK_LE(byte_range_.first_byte_position(),
            byte_range_.last_byte_position() + 1);
  uint64_t length = base::checked_cast<uint64_t>(
      byte_range_.last_byte_position() - byte_range_.first_byte_position() + 1);

  if (blob_reader_->SetReadRange(byte_range_.first_byte_position(), length) !=
      BlobReader::Status::DONE) {
    NotifyCompleted(blob_reader_->net_error());
    return;
  }

  // TODO(mek): Read side data if needed.
  if (delegate_->DidCalculateSize(blob_reader_->total_size(),
                                  blob_reader_->remaining_bytes()) ==
      Delegate::REQUEST_SIDE_DATA) {
    if (!blob_reader_->has_side_data()) {
      DidReadSideData(BlobReader::Status::DONE);
    } else {
      BlobReader::Status read_status = blob_reader_->ReadSideData(
          base::Bind(&MojoBlobReader::DidReadSideData, base::Unretained(this)));
      if (read_status != BlobReader::Status::IO_PENDING)
        DidReadSideData(BlobReader::Status::DONE);
    }
  } else {
    StartReading();
  }
}

void MojoBlobReader::DidReadSideData(BlobReader::Status status) {
  if (status != BlobReader::Status::DONE) {
    NotifyCompleted(blob_reader_->net_error());
    return;
  }
  delegate_->DidReadSideData(blob_reader_->side_data());
  StartReading();
}

void MojoBlobReader::StartReading() {
  response_body_stream_ = delegate_->GetDataPipe();
  peer_closed_handle_watcher_.Watch(
      response_body_stream_.get(), MOJO_HANDLE_SIGNAL_PEER_CLOSED,
      base::Bind(&MojoBlobReader::OnResponseBodyStreamClosed,
                 base::Unretained(this)));
  peer_closed_handle_watcher_.ArmOrNotify();

  writable_handle_watcher_.Watch(
      response_body_stream_.get(), MOJO_HANDLE_SIGNAL_WRITABLE,
      base::Bind(&MojoBlobReader::OnResponseBodyStreamReady,
                 base::Unretained(this)));

  // Start reading...
  ReadMore();
}

void MojoBlobReader::ReadMore() {
  DCHECK(!pending_write_.get());

  uint32_t num_bytes;
  // TODO: we should use the abstractions in MojoAsyncResourceHandler.
  MojoResult result = NetToMojoPendingBuffer::BeginWrite(
      &response_body_stream_, &pending_write_, &num_bytes);
  if (result == MOJO_RESULT_SHOULD_WAIT) {
    // The pipe is full. We need to wait for it to have more space.
    writable_handle_watcher_.ArmOrNotify();
    return;
  } else if (result != MOJO_RESULT_OK) {
    // The response body stream is in a bad state. Bail.
    writable_handle_watcher_.Cancel();
    response_body_stream_.reset();
    NotifyCompleted(net::ERR_UNEXPECTED);
    return;
  }

  CHECK_GT(static_cast<uint32_t>(std::numeric_limits<int>::max()), num_bytes);
  scoped_refptr<net::IOBuffer> buf(new NetToMojoIOBuffer(pending_write_.get()));
  int bytes_read;
  BlobReader::Status read_status = blob_reader_->Read(
      buf.get(), static_cast<int>(num_bytes), &bytes_read,
      base::Bind(&MojoBlobReader::DidRead, base::Unretained(this), false));
  switch (read_status) {
    case BlobReader::Status::NET_ERROR:
      NotifyCompleted(blob_reader_->net_error());
      return;
    case BlobReader::Status::IO_PENDING:
      // Wait for DidRead.
      return;
    case BlobReader::Status::DONE:
      if (bytes_read > 0) {
        DidRead(true, bytes_read);
      } else {
        writable_handle_watcher_.Cancel();
        pending_write_->Complete(0);
        pending_write_ = nullptr;  // This closes the data pipe.
        NotifyCompleted(net::OK);
        return;
      }
  }
}

void MojoBlobReader::DidRead(bool completed_synchronously, int num_bytes) {
  delegate_->DidRead(num_bytes);
  response_body_stream_ = pending_write_->Complete(num_bytes);
  total_written_bytes_ += num_bytes;
  pending_write_ = nullptr;
  if (completed_synchronously) {
    base::SequencedTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::Bind(&MojoBlobReader::ReadMore, weak_factory_.GetWeakPtr()));
  } else {
    ReadMore();
  }
}

void MojoBlobReader::OnResponseBodyStreamClosed(MojoResult result) {
  response_body_stream_.reset();
  pending_write_ = nullptr;
  NotifyCompleted(net::ERR_ABORTED);
}

void MojoBlobReader::OnResponseBodyStreamReady(MojoResult result) {
  // TODO: Handle a bad |result| value.
  DCHECK_EQ(result, MOJO_RESULT_OK);
  ReadMore();
}

}  // namespace storage
