// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/common/mojo_data_buffer_converter.h"

#include <memory>

#include "base/logging.h"
#include "media/base/data_buffer.h"
#include "media/mojo/common/media_type_converters.h"

namespace media {

namespace {

std::unique_ptr<mojo::DataPipe> CreateDataPipe(uint32_t capacity_num_bytes) {
  MojoCreateDataPipeOptions options;
  options.struct_size = sizeof(MojoCreateDataPipeOptions);
  options.flags = MOJO_CREATE_DATA_PIPE_OPTIONS_FLAG_NONE;
  options.element_num_bytes = 1;
  options.capacity_num_bytes = capacity_num_bytes;
  return base::MakeUnique<mojo::DataPipe>(options);
}

bool IsPipeReadWriteError(MojoResult result) {
  return result != MOJO_RESULT_OK && result != MOJO_RESULT_SHOULD_WAIT;
}

}  // namespace

// MojoDataBufferReader

// static
std::unique_ptr<MojoDataBufferReader> MojoDataBufferReader::Create(
    uint32_t capacity_num_bytes,
    mojo::ScopedDataPipeProducerHandle* producer_handle) {
  DVLOG(1) << __func__;
  std::unique_ptr<mojo::DataPipe> data_pipe =
      CreateDataPipe(capacity_num_bytes);
  *producer_handle = std::move(data_pipe->producer_handle);
  return base::WrapUnique(
      new MojoDataBufferReader(std::move(data_pipe->consumer_handle)));
}

MojoDataBufferReader::MojoDataBufferReader(
    mojo::ScopedDataPipeConsumerHandle consumer_handle)
    : consumer_handle_(std::move(consumer_handle)),
      pipe_watcher_(FROM_HERE, mojo::SimpleWatcher::ArmingPolicy::MANUAL),
      bytes_read_(0) {
  DVLOG(1) << __func__;

  MojoResult result =
      pipe_watcher_.Watch(consumer_handle_.get(), MOJO_HANDLE_SIGNAL_READABLE,
                          base::Bind(&MojoDataBufferReader::OnPipeReadable,
                                     base::Unretained(this)));
  if (result != MOJO_RESULT_OK) {
    DVLOG(1) << __func__
             << ": Failed to start watching the pipe. result=" << result;
    consumer_handle_.reset();
  }
}

MojoDataBufferReader::~MojoDataBufferReader() {
  DVLOG(1) << __func__;
}

void MojoDataBufferReader::ReadDataBuffer(mojom::DataBufferPtr mojo_buffer,
                                          ReadCB read_cb) {
  DVLOG(3) << __func__;

  // DataBuffer cannot be read if the pipe is already closed.
  if (!consumer_handle_.is_valid()) {
    DVLOG(1)
        << __func__
        << ": Failed to read DataBuffer becuase the pipe is already closed";
    std::move(read_cb).Run(nullptr);
    return;
  }

  DCHECK(!read_cb_);
  DCHECK(!media_buffer_);
  DCHECK_EQ(bytes_read_, 0u);

  scoped_refptr<DataBuffer> media_buffer(
      mojo_buffer.To<scoped_refptr<DataBuffer>>());
  DCHECK(media_buffer);

  // A non-EOS buffer can have zero size. See http://crbug.com/663438
  if (media_buffer->end_of_stream() || media_buffer->data_size() == 0) {
    std::move(read_cb).Run(media_buffer);
    return;
  }

  // Read the data section of |media_buffer| from the pipe.
  read_cb_ = std::move(read_cb);
  media_buffer_ = std::move(media_buffer);
  ReadDataBufferData();
}

void MojoDataBufferReader::OnPipeError(MojoResult result) {
  DVLOG(1) << __func__ << "(" << result << ")";
  DCHECK(IsPipeReadWriteError(result));

  if (media_buffer_) {
    DVLOG(1) << __func__ << ": reading from data pipe failed. result=" << result
             << ", buffer size=" << media_buffer_->data_size()
             << ", num_bytes(read)=" << bytes_read_;
    DCHECK(read_cb_);
    bytes_read_ = 0;
    media_buffer_ = nullptr;
    std::move(read_cb_).Run(nullptr);
  }
  consumer_handle_.reset();
}

void MojoDataBufferReader::OnPipeReadable(MojoResult result) {
  DVLOG(4) << __func__ << "(" << result << ")";

  if (result != MOJO_RESULT_OK)
    OnPipeError(result);
  else if (media_buffer_)
    ReadDataBufferData();
}

void MojoDataBufferReader::ReadDataBufferData() {
  DVLOG(4) << __func__;
  DCHECK(media_buffer_);

  uint32_t buffer_size =
      base::checked_cast<uint32_t>(media_buffer_->data_size());
  DCHECK_GT(buffer_size, 0u);

  uint32_t num_bytes = buffer_size - bytes_read_;
  DCHECK_GT(num_bytes, 0u);

  MojoResult result =
      consumer_handle_->ReadData(media_buffer_->writable_data() + bytes_read_,
                                 &num_bytes, MOJO_WRITE_DATA_FLAG_NONE);

  if (IsPipeReadWriteError(result)) {
    OnPipeError(result);
  } else if (result == MOJO_RESULT_OK) {
    DCHECK_GT(num_bytes, 0u);
    bytes_read_ += num_bytes;
    if (bytes_read_ == buffer_size) {
      DCHECK(read_cb_);
      bytes_read_ = 0;
      std::move(read_cb_).Run(std::move(media_buffer_));
    }
  }
}

// MojoDataBufferWriter

// static
std::unique_ptr<MojoDataBufferWriter> MojoDataBufferWriter::Create(
    uint32_t capacity_num_bytes,
    mojo::ScopedDataPipeConsumerHandle* consumer_handle) {
  DVLOG(1) << __func__;
  std::unique_ptr<mojo::DataPipe> data_pipe =
      CreateDataPipe(capacity_num_bytes);
  *consumer_handle = std::move(data_pipe->consumer_handle);
  return base::WrapUnique(
      new MojoDataBufferWriter(std::move(data_pipe->producer_handle)));
}

MojoDataBufferWriter::MojoDataBufferWriter(
    mojo::ScopedDataPipeProducerHandle producer_handle)
    : producer_handle_(std::move(producer_handle)),
      pipe_watcher_(FROM_HERE, mojo::SimpleWatcher::ArmingPolicy::MANUAL),
      bytes_written_(0) {
  DVLOG(1) << __func__;

  MojoResult result =
      pipe_watcher_.Watch(producer_handle_.get(), MOJO_HANDLE_SIGNAL_WRITABLE,
                          base::Bind(&MojoDataBufferWriter::OnPipeWritable,
                                     base::Unretained(this)));
  if (result != MOJO_RESULT_OK) {
    DVLOG(1) << __func__
             << ": Failed to start watching the pipe. result=" << result;
    producer_handle_.reset();
  }
}

MojoDataBufferWriter::~MojoDataBufferWriter() {
  DVLOG(1) << __func__;
}

mojom::DataBufferPtr MojoDataBufferWriter::WriteDataBuffer(
    const scoped_refptr<DataBuffer>& media_buffer) {
  DVLOG(3) << __func__;

  // DataBuffer cannot be written if the pipe is already closed.
  if (!producer_handle_.is_valid()) {
    DVLOG(1)
        << __func__
        << ": Failed to write DataBuffer becuase the pipe is already closed";
    return nullptr;
  }

  DCHECK(!media_buffer_);
  DCHECK_EQ(bytes_written_, 0u);

  mojom::DataBufferPtr mojo_buffer = mojom::DataBuffer::From(media_buffer);

  // A non-EOS buffer can have zero size. See http://crbug.com/663438
  if (media_buffer->end_of_stream() || media_buffer->data_size() == 0)
    return mojo_buffer;

  // Serialize the data section of the DataBuffer into our pipe.
  media_buffer_ = media_buffer;
  MojoResult result = WriteDataBufferData();
  return IsPipeReadWriteError(result) ? nullptr : std::move(mojo_buffer);
}

void MojoDataBufferWriter::OnPipeError(MojoResult result) {
  DVLOG(1) << __func__ << "(" << result << ")";
  DCHECK(IsPipeReadWriteError(result));

  if (media_buffer_) {
    DVLOG(1) << __func__ << ": writing to data pipe failed. result=" << result
             << ", buffer size=" << media_buffer_->data_size()
             << ", num_bytes(written)=" << bytes_written_;
    media_buffer_ = nullptr;
    bytes_written_ = 0;
  }
  producer_handle_.reset();
}

void MojoDataBufferWriter::OnPipeWritable(MojoResult result) {
  DVLOG(4) << __func__ << "(" << result << ")";

  if (result != MOJO_RESULT_OK)
    OnPipeError(result);
  else if (media_buffer_)
    WriteDataBufferData();
}

MojoResult MojoDataBufferWriter::WriteDataBufferData() {
  DVLOG(4) << __func__;
  DCHECK(media_buffer_);

  uint32_t buffer_size =
      base::checked_cast<uint32_t>(media_buffer_->data_size());
  DCHECK_GT(buffer_size, 0u);

  uint32_t num_bytes = buffer_size - bytes_written_;
  DCHECK_GT(num_bytes, 0u);

  MojoResult result =
      producer_handle_->WriteData(media_buffer_->data() + bytes_written_,
                                  &num_bytes, MOJO_WRITE_DATA_FLAG_NONE);

  if (IsPipeReadWriteError(result)) {
    OnPipeError(result);
  } else if (result == MOJO_RESULT_OK) {
    DCHECK_GT(num_bytes, 0u);
    bytes_written_ += num_bytes;
    if (bytes_written_ == buffer_size) {
      media_buffer_ = nullptr;
      bytes_written_ = 0;
    }
  }

  return result;
}

}  // namespace media
