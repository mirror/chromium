// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MOJO_COMMON_MOJO_DECODER_BUFFER_CONVERTER_
#define MEDIA_MOJO_COMMON_MOJO_DECODER_BUFFER_CONVERTER_

#include <memory>

#include "base/containers/circular_deque.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "media/base/demuxer_stream.h"
#include "media/mojo/interfaces/media_types.mojom.h"
#include "mojo/public/cpp/system/data_pipe.h"
#include "mojo/public/cpp/system/simple_watcher.h"

namespace media {

class DecoderBuffer;

// A helper class that converts mojom::DecoderBuffer to media::DecoderBuffer.
// The data part of the DecoderBuffer is read from a DataPipe.
class MojoDecoderBufferReader {
 public:
  using ReadCB = base::OnceCallback<void(scoped_refptr<DecoderBuffer>)>;

  // Creates a MojoDecoderBufferReader of |type| and set the |producer_handle|.
  static std::unique_ptr<MojoDecoderBufferReader> Create(
      DemuxerStream::Type type,
      mojo::ScopedDataPipeProducerHandle* producer_handle);

  // Hold the consumer handle to read DecoderBuffer data.
  explicit MojoDecoderBufferReader(
      mojo::ScopedDataPipeConsumerHandle consumer_handle);

  ~MojoDecoderBufferReader();

  // Converts |buffer| into a DecoderBuffer, reading data from DataPipe if
  // needed). |read_cb| is called with the result DecoderBuffer. Reports a null
  // DecoderBuffer in case of an error.
  void ReadDecoderBuffer(mojom::DecoderBufferPtr buffer, ReadCB read_cb);

 private:
  void CancelReadCB(ReadCB read_cb);
  void CancelAllPendingReads();
  void CompleteEmptyReads();
  void CompleteCurrentRead();
  void OnPipeReadable(MojoResult result, const mojo::HandleSignalsState& state);
  void ReadDecoderBufferData();
  void OnPipeError(MojoResult result);

  // Read side of the DataPipe for receiving DecoderBuffer data.
  mojo::ScopedDataPipeConsumerHandle consumer_handle_;

  // Provides notification about |consumer_handle_| readiness.
  mojo::SimpleWatcher pipe_watcher_;

  // Buffers waiting to be read in sequence.
  base::circular_deque<scoped_refptr<DecoderBuffer>> pending_buffers_;

  // Callbacks for pending buffers.
  base::circular_deque<ReadCB> pending_read_cbs_;

  // Number of bytes already read into the current buffer.
  uint32_t bytes_read_;

  DISALLOW_COPY_AND_ASSIGN(MojoDecoderBufferReader);
};

// A helper class that converts media::DecoderBuffer to mojom::DecoderBuffer.
// The data part of the DecoderBuffer is written into a DataPipe.
class MojoDecoderBufferWriter {
 public:
  // Creates a MojoDecoderBufferWriter of |type| and set the |consumer_handle|.
  static std::unique_ptr<MojoDecoderBufferWriter> Create(
      DemuxerStream::Type type,
      mojo::ScopedDataPipeConsumerHandle* consumer_handle);

  // Hold the producer handle to write DecoderBuffer data.
  explicit MojoDecoderBufferWriter(
      mojo::ScopedDataPipeProducerHandle producer_handle);

  ~MojoDecoderBufferWriter();

  // Converts a DecoderBuffer into mojo DecoderBuffer.
  // DecoderBuffer data is asynchronously written into DataPipe if needed.
  // Returns null if conversion failed or if the data pipe is already closed.
  mojom::DecoderBufferPtr WriteDecoderBuffer(
      const scoped_refptr<DecoderBuffer>& media_buffer);

 private:
  void OnPipeWritable(MojoResult result, const mojo::HandleSignalsState& state);
  void WriteDecoderBufferData();
  void OnPipeError(MojoResult result);

  // Write side of the DataPipe for sending DecoderBuffer data.
  mojo::ScopedDataPipeProducerHandle producer_handle_;

  // Provides notifications about |producder_handle_| readiness.
  mojo::SimpleWatcher pipe_watcher_;

  // Buffers waiting to be written in sequence.
  base::circular_deque<scoped_refptr<DecoderBuffer>> pending_buffers_;

  // Number of bytes already written from the current buffer.
  uint32_t bytes_written_;

  DISALLOW_COPY_AND_ASSIGN(MojoDecoderBufferWriter);
};

}  // namespace media

#endif  // MEDIA_MOJO_COMMON_MOJO_DECODER_BUFFER_CONVERTER_
