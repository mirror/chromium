// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/common/mojo_decoder_buffer_converter.h"

#include <memory>

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "media/base/audio_buffer.h"
#include "media/base/cdm_context.h"
#include "media/base/decoder_buffer.h"
#include "media/mojo/common/media_type_converters.h"
#include "media/mojo/common/mojo_data_pipe_read_write.h"

namespace media {

namespace {

std::unique_ptr<mojo::DataPipe> CreateDataPipe(DemuxerStream::Type type) {
  uint32_t capacity = 0;

  if (type == DemuxerStream::AUDIO) {
    // TODO(timav): Consider capacity calculation based on AudioDecoderConfig.
    capacity = 512 * 1024;
  } else if (type == DemuxerStream::VIDEO) {
    // Video can get quite large; at 4K, VP9 delivers packets which are ~1MB in
    // size; so allow for some head room.
    // TODO(xhwang, sandersd): Provide a better way to customize this value.
    capacity = 2 * (1024 * 1024);
  } else {
    NOTREACHED() << "Unsupported type: " << type;
    // Choose an arbitrary size.
    capacity = 512 * 1024;
  }

  return base::MakeUnique<mojo::DataPipe>(capacity);
}

}  // namespace

// MojoDecoderBufferReader

// static
std::unique_ptr<MojoDecoderBufferReader> MojoDecoderBufferReader::Create(
    DemuxerStream::Type type,
    mojo::ScopedDataPipeProducerHandle* producer_handle) {
  DVLOG(1) << __func__;
  std::unique_ptr<mojo::DataPipe> data_pipe = CreateDataPipe(type);
  *producer_handle = std::move(data_pipe->producer_handle);
  return base::WrapUnique(
      new MojoDecoderBufferReader(std::move(data_pipe->consumer_handle)));
}

MojoDecoderBufferReader::MojoDecoderBufferReader(
    mojo::ScopedDataPipeConsumerHandle consumer_handle)
    : data_pipe_reader_(new MojoDataPipeReader(std::move(consumer_handle))),
      armed_(false) {
  DVLOG(1) << __func__;
}

MojoDecoderBufferReader::~MojoDecoderBufferReader() {
  DVLOG(1) << __func__;
}

void MojoDecoderBufferReader::CancelReadCB(ReadCB read_cb) {
  DVLOG(1) << "Failed to read DecoderBuffer becuase the pipe is already closed";
  std::move(read_cb).Run(nullptr);
}

void MojoDecoderBufferReader::CompleteCurrentRead() {
  DVLOG(4) << __func__;
  ReadCB read_cb = std::move(pending_read_cbs_.front());
  pending_read_cbs_.pop_front();
  scoped_refptr<DecoderBuffer> buffer = std::move(pending_buffers_.front());
  pending_buffers_.pop_front();
  std::move(read_cb).Run(std::move(buffer));
}

void MojoDecoderBufferReader::ScheduleNextRead() {
  if (armed_)
    return;

  // Immediately complete empty reads.
  // A non-EOS buffer can have zero size. See http://crbug.com/663438
  while (!pending_buffers_.empty() &&
         (pending_buffers_.front()->end_of_stream() ||
          pending_buffers_.front()->data_size() == 0)) {
    // TODO(sandersd): Make sure there are no possible re-entrancy issues here.
    // Perhaps read callbacks should be posted?
    CompleteCurrentRead();
  }

  // Request to read the buffer.
  if (!pending_buffers_.empty()) {
    armed_ = true;
    data_pipe_reader_->Read(
        pending_buffers_.front()->writable_data(),
        pending_buffers_.front()->data_size(),
        base::BindOnce(&MojoDecoderBufferReader::OnBufferRead,
                       base::Unretained(this)));
  }
}

void MojoDecoderBufferReader::ReadDecoderBuffer(
    mojom::DecoderBufferPtr mojo_buffer,
    ReadCB read_cb) {
  DVLOG(3) << __func__;

  if (!data_pipe_reader_->IsPipeValid()) {
    DCHECK(pending_read_cbs_.empty());
    CancelReadCB(std::move(read_cb));
    return;
  }

  scoped_refptr<DecoderBuffer> media_buffer(
      mojo_buffer.To<scoped_refptr<DecoderBuffer>>());
  DCHECK(media_buffer);

  // We don't want reads to complete out of order, so we queue them even if they
  // are zero-sized.
  pending_read_cbs_.push_back(std::move(read_cb));
  pending_buffers_.push_back(std::move(media_buffer));
  ScheduleNextRead();
}

void MojoDecoderBufferReader::OnBufferRead(bool success) {
  DVLOG(4) << __func__;
  if (!success) {
    OnError();
    return;
  }
  armed_ = false;
  CompleteCurrentRead();
  ScheduleNextRead();
}

void MojoDecoderBufferReader::OnError() {
  DVLOG(1) << __func__;
  if (!pending_buffers_.empty()) {
    DVLOG(1) << __func__ << ": reading from data pipe failed."
             << ", buffer size=" << pending_buffers_.front()->data_size();
    pending_buffers_.clear();
    while (!pending_read_cbs_.empty()) {
      ReadCB read_cb = std::move(pending_read_cbs_.front());
      pending_read_cbs_.pop_front();
      // TODO(sandersd): Make sure there are no possible re-entrancy issues
      // here. Perhaps these should be posted, or merged into a single error
      // callback?
      CancelReadCB(std::move(read_cb));
    }
  }
}

// MojoDecoderBufferWriter

// static
std::unique_ptr<MojoDecoderBufferWriter> MojoDecoderBufferWriter::Create(
    DemuxerStream::Type type,
    mojo::ScopedDataPipeConsumerHandle* consumer_handle) {
  DVLOG(1) << __func__;
  std::unique_ptr<mojo::DataPipe> data_pipe = CreateDataPipe(type);
  *consumer_handle = std::move(data_pipe->consumer_handle);
  return base::WrapUnique(
      new MojoDecoderBufferWriter(std::move(data_pipe->producer_handle)));
}

MojoDecoderBufferWriter::MojoDecoderBufferWriter(
    mojo::ScopedDataPipeProducerHandle producer_handle)
    : data_pipe_writer_(new MojoDataPipeWriter(std::move(producer_handle))) {
  DVLOG(1) << __func__;
}

MojoDecoderBufferWriter::~MojoDecoderBufferWriter() {
  DVLOG(1) << __func__;
}

void MojoDecoderBufferWriter::ScheduleNextWrite() {
  DVLOG(4) << __func__;

  // Do nothing if a write is already scheduled.
  if (armed_)
    return;

  // Request to write the buffer to the data pipe.
  if (!pending_buffers_.empty()) {
    armed_ = true;
    data_pipe_writer_->Write(
        pending_buffers_.front()->data(), pending_buffers_.front()->data_size(),
        base::BindOnce(&MojoDecoderBufferWriter::OnBufferWritten,
                       base::Unretained(this)));
  }
}

mojom::DecoderBufferPtr MojoDecoderBufferWriter::WriteDecoderBuffer(
    const scoped_refptr<DecoderBuffer>& media_buffer) {
  DVLOG(3) << __func__;

  // DecoderBuffer cannot be written if the pipe is already closed.
  if (!data_pipe_writer_->IsPipeValid()) {
    DVLOG(1)
        << __func__
        << ": Failed to write DecoderBuffer becuase the pipe is already closed";
    return nullptr;
  }

  mojom::DecoderBufferPtr mojo_buffer =
      mojom::DecoderBuffer::From(media_buffer);

  // A non-EOS buffer can have zero size. See http://crbug.com/663438
  if (media_buffer->end_of_stream() || media_buffer->data_size() == 0)
    return mojo_buffer;

  // Queue writing the buffer's data into our DataPipe.
  pending_buffers_.push_back(media_buffer);
  ScheduleNextWrite();
  return mojo_buffer;
}

void MojoDecoderBufferWriter::OnBufferWritten(bool success) {
  armed_ = false;

  if (!success) {
    OnError();
    return;
  }

  pending_buffers_.pop_front();
  ScheduleNextWrite();
}

void MojoDecoderBufferWriter::OnError() {
  DVLOG(1) << __func__;

  if (!pending_buffers_.empty()) {
    DVLOG(1) << __func__ << ": writing to data pipe failed."
             << " buffer size=" << pending_buffers_.front()->data_size();
    pending_buffers_.clear();
  }
}

}  // namespace media
