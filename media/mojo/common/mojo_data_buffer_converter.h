// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MOJO_COMMON_MOJO_DATA_BUFFER_CONVERTER_
#define MEDIA_MOJO_COMMON_MOJO_DATA_BUFFER_CONVERTER_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "media/mojo/interfaces/media_types.mojom.h"
#include "mojo/public/cpp/system/data_pipe.h"
#include "mojo/public/cpp/system/simple_watcher.h"

namespace media {

class DataBuffer;

// A helper class that converts mojom::DataBuffer to media::DataBuffer.
// The data part of the DataBuffer is read from a DataPipe.
class MojoDataBufferReader {
 public:
  using ReadCB = base::OnceCallback<void(scoped_refptr<DataBuffer>)>;

  // Creates a MojoDataBufferReader of |type| and set the |producer_handle|.
  static std::unique_ptr<MojoDataBufferReader> Create(
      uint32_t capacity_num_bytes,
      mojo::ScopedDataPipeProducerHandle* producer_handle);

  // Hold the consumer handle to read DataBuffer data.
  explicit MojoDataBufferReader(
      mojo::ScopedDataPipeConsumerHandle consumer_handle);

  ~MojoDataBufferReader();

  // Converts |buffer| into a DataBuffer (read data from DataPipe if needed).
  // |read_cb| is called with the result DataBuffer.
  // Reports a null DataBuffer in case of an error.
  void ReadDataBuffer(mojom::DataBufferPtr buffer, ReadCB read_cb);

 private:
  void OnPipeError(MojoResult result);
  void OnPipeReadable(MojoResult result);
  void ReadDataBufferData();

  // For reading the data section of a DataBuffer.
  mojo::ScopedDataPipeConsumerHandle consumer_handle_;
  mojo::SimpleWatcher pipe_watcher_;

  // Only valid during pending read.
  ReadCB read_cb_;
  scoped_refptr<DataBuffer> media_buffer_;
  uint32_t bytes_read_;

  DISALLOW_COPY_AND_ASSIGN(MojoDataBufferReader);
};

// A helper class that converts media::DataBuffer to mojom::DataBuffer.
// The data part of the DataBuffer is written into a DataPipe.
class MojoDataBufferWriter {
 public:
  // Creates a MojoDataBufferWriter of |type| and set the |consumer_handle|.
  static std::unique_ptr<MojoDataBufferWriter> Create(
      uint32_t capacity_num_bytes,
      mojo::ScopedDataPipeConsumerHandle* consumer_handle);

  // Hold the producer handle to write DataBuffer data.
  explicit MojoDataBufferWriter(
      mojo::ScopedDataPipeProducerHandle producer_handle);

  ~MojoDataBufferWriter();

  // Converts a DataBuffer into mojo DataBuffer.
  // DataBuffer data is asynchronously written into DataPipe if needed.
  // Returns null if conversion failed or if the data pipe is already closed.
  mojom::DataBufferPtr WriteDataBuffer(
      const scoped_refptr<DataBuffer>& media_buffer);

 private:
  void OnPipeError(MojoResult result);
  void OnPipeWritable(MojoResult result);
  MojoResult WriteDataBufferData();

  // For writing the data section of DataBuffer into DataPipe.
  mojo::ScopedDataPipeProducerHandle producer_handle_;
  mojo::SimpleWatcher pipe_watcher_;

  // Only valid when data is being written to the pipe.
  scoped_refptr<DataBuffer> media_buffer_;
  uint32_t bytes_written_;

  DISALLOW_COPY_AND_ASSIGN(MojoDataBufferWriter);
};

}  // namespace media

#endif  // MEDIA_MOJO_COMMON_MOJO_DATA_BUFFER_CONVERTER_
