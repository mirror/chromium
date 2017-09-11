// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_LOADER_MOJO_PIPE_WRITER_H_
#define CONTENT_BROWSER_LOADER_MOJO_PIPE_WRITER_H_

#include <stdint.h>

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "content/common/content_export.h"
#include "mojo/public/cpp/bindings/associated_binding.h"
#include "mojo/public/cpp/system/data_pipe.h"
#include "mojo/public/cpp/system/simple_watcher.h"
#include "net/base/io_buffer.h"
#include "net/base/net_errors.h"
#include "net/base/request_priority.h"

namespace net {
class IOBufferWithSize;
}  // namespace net

namespace content {

// This class is used to write and read data in a Mojo pipe as part as
// asynchronous resource requests between the browser and the renderer.
// This class should only be derived in tests.
class CONTENT_EXPORT MojoPipeWriter {
 public:
  class Delegate {
   public:
    // Called when an asynchronous operation of the PipeWriter has completed.
    virtual void OnPipeWriterOperationComplete(net::Error result) = 0;
  };

  // |delegate| and |consumer_handle| should outlive this writer.
  MojoPipeWriter(MojoPipeWriter::Delegate* delegate,
                 mojo::ScopedDataPipeConsumerHandle* consumer_handle);
  virtual ~MojoPipeWriter();

  // Initializes and returns the input data buffer for the data pipe. Once the
  // buffer has been initialized, |data_buffer| and |buf_size| will be updated
  // to match the newly initialized data buffer. Data that needs to be written
  // into the pipe should be placed into this data buffer before calling
  // WriteFromDataBuffer.
  net::Error InitializeDataBuffer(scoped_refptr<net::IOBuffer>* data_buffer,
                                  int* buf_size);

  // Writes data from the input data buffer into the data pipe.
  net::Error WriteFromDataBuffer(int bytes_to_write);

  // Initializes the data pipe if it has not been initialized yet.
  net::Error InitializeDataPipeIfNeeded();

  // Finalizes the data pipe. It is no longer possible to write into the data
  // pipe after calling this function.
  void Finalize();

  int64_t total_written_bytes() const { return total_written_bytes_; }

  void OnWritableForTesting();
  static void SetAllocationSizeForTesting(size_t size);
  static constexpr size_t kDefaultAllocationSize = 512 * 1024;

 protected:
  // These functions should only be overridden in tests.
  virtual MojoResult BeginWrite(void** data, uint32_t* available);
  virtual MojoResult EndWrite(uint32_t written);

 private:
  class SharedWriter;
  class WriterIOBuffer;

  // This function copies data stored in |buffer_| to |shared_writer_| and
  // resets |buffer_| to a WriterIOBuffer when all bytes are copied. Returns
  // true when done successfully.
  bool CopyReadDataToDataPipe(bool* defer);
  // Allocates a WriterIOBuffer and set it to |*buf|. Returns true when done
  // successfully.
  bool AllocateWriterIOBuffer(scoped_refptr<net::IOBufferWithSize>* buf,
                              bool* defer);

  void OnWritable(MojoResult result);

  Delegate* delegate_;

  bool is_using_io_buffer_not_from_writer_ = false;

  // True if InitializeDataBuffer was deferred, in order to wait to be able to
  // allocate a buffer.
  bool did_defer_on_initializing_buffer_ = false;
  // True if WriteFromDataBuffer was deferred.
  bool did_defer_on_writing_ = false;

  bool first_write_ = true;

  // Pointer to information about the input data buffer provided by the caller
  // of InitializeDataBuffer. Only non-null while InitializeDataBuffer is
  // deferred.
  scoped_refptr<net::IOBuffer>* provided_buffer_ = nullptr;
  int* provided_buffer_size_ = nullptr;

  int64_t total_written_bytes_ = 0;

  mojo::SimpleWatcher handle_watcher_;
  scoped_refptr<net::IOBufferWithSize> buffer_;
  size_t buffer_offset_ = 0;
  size_t buffer_bytes_read_ = 0;
  scoped_refptr<SharedWriter> shared_writer_;
  mojo::ScopedDataPipeConsumerHandle* consumer_handle_;

  base::WeakPtrFactory<MojoPipeWriter> weak_factory_;
  DISALLOW_COPY_AND_ASSIGN(MojoPipeWriter);
};

}  // namespace content

#endif  // CONTENT_BROWSER_LOADER_MOJO_PIPE_WRITER_H_
