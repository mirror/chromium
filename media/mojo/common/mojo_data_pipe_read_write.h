// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MOJO_COMMON_MOJO_DATA_PIPE_READ_WRITE_H_
#define MEDIA_MOJO_COMMON_MOJO_DATA_PIPE_READ_WRITE_H_

#include <memory>

#include "base/containers/circular_deque.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "mojo/public/cpp/system/data_pipe.h"
#include "mojo/public/cpp/system/simple_watcher.h"

namespace media {

// An async reader to read a certain amount of bytes from a mojo data pipe by
// request.
class MojoDataPipeReader {
 public:
  // Hold the consumer handle to read data. The |read_cb| will be called in
  // CompleteCurrentRead() when a read is completed.
  explicit MojoDataPipeReader(
      mojo::ScopedDataPipeConsumerHandle consumer_handle);

  ~MojoDataPipeReader();

  using DoneCB = base::OnceCallback<void(bool)>;
  // Reads |num_bytes| data from the mojo data pipe into |buffer|. When
  // completes, |done_cb| is called and indicates whether the reading succeeds.
  // This is not allowed to be called when a reading is ongoing. |buffer| needs
  // to be valid for writing for the whole reading process.
  void Read(uint8_t* buffer, uint32_t num_bytes, DoneCB done_cb);

  bool IsPipeValid() const;

 private:
  void CompleteCurrentRead();
  void OnPipeReadable(MojoResult result, const mojo::HandleSignalsState& state);
  void OnPipeError(MojoResult result);

  // Read side of the data pipe.
  mojo::ScopedDataPipeConsumerHandle consumer_handle_;

  // Provides notification about |consumer_handle_| readiness.
  mojo::SimpleWatcher pipe_watcher_;

  // The current buffer to be read. It is provided by Read() and should be
  // guaranteed to be valid until the current read completes.
  uint8_t* current_buffer_ = nullptr;

  // The number of bytes to be read for the current read request.
  uint32_t current_buffer_size_ = 0;

  // The current once callback to be called when read completes.
  DoneCB done_cb_;

  // Number of bytes already read into the current buffer.
  uint32_t bytes_read_ = 0;

  DISALLOW_COPY_AND_ASSIGN(MojoDataPipeReader);
};

// An async writer to write a certain amount of data into a mojo data pipe by
// request.
class MojoDataPipeWriter {
 public:
  // Hold the producer handle to write data.
  explicit MojoDataPipeWriter(
      mojo::ScopedDataPipeProducerHandle producer_handle);

  ~MojoDataPipeWriter();

  using DoneCB = base::OnceCallback<void(bool)>;
  // Writes |num_bytes| data from |buffer| into the mojo data pipe. When
  // completes, |done_cb| is called and indicates whether the writing succeeds.
  // This is not allowed to be called when a writing is ongoing. |buffer| needs
  // to be valid for reading for the entire writing process.
  void Write(const uint8_t* buffer, uint32_t num_bytes, DoneCB done_cb);

  bool IsPipeValid() const;

 private:
  void OnPipeWritable(MojoResult result, const mojo::HandleSignalsState& state);
  void OnPipeError(MojoResult result);
  void CompleteCurrentWrite();

  // Write side of the data pipe.
  mojo::ScopedDataPipeProducerHandle producer_handle_;

  // Provides notifications about |producer_handle_| readiness.
  mojo::SimpleWatcher pipe_watcher_;

  // The current buffer to be written. It is provided by Write() and should be
  // guaranteed to be valid until the current write completes.
  const uint8_t* current_buffer_ = nullptr;

  // The number of bytes to be written for the current write request.
  uint32_t current_buffer_size_ = 0;

  // The current once callback to be called when write completes.
  DoneCB done_cb_;

  // Number of bytes already written from the current buffer.
  uint32_t bytes_written_ = 0;

  DISALLOW_COPY_AND_ASSIGN(MojoDataPipeWriter);
};

}  // namespace media

#endif  // MEDIA_MOJO_COMMON_MOJO_DATA_PIPE_READ_WRITE_H_
