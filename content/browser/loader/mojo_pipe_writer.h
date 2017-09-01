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
#include "content/browser/loader/resource_handler.h"
#include "content/browser/loader/upload_progress_tracker.h"
#include "content/common/content_export.h"
#include "content/public/common/resource_type.h"
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
    virtual void OnPipeWriterOperationComplete(net::Error result) = 0;
  };
  MojoPipeWriter(MojoPipeWriter::Delegate* delegate,
                 mojo::ScopedDataPipeConsumerHandle* consumer_handle);
  virtual ~MojoPipeWriter();

  net::Error OnWillRead(scoped_refptr<net::IOBuffer>* buf, int* buf_size);
  net::Error OnReadCompleted(int bytes_read);
  net::Error InitializeDataPipeIfNeeded();
  void Finalize();

  int64_t total_written_bytes() const { return total_written_bytes_; }

  void OnWritableForTesting();
  static void SetAllocationSizeForTesting(size_t size);
  static constexpr size_t kDefaultAllocationSize = 512 * 1024;

 protected:
  // These functions should only be overriden in tests.
  virtual MojoResult BeginWrite(void** data, uint32_t* available);
  virtual MojoResult EndWrite(uint32_t written);

 private:
  class SharedWriter;
  class WriterIOBuffer;

  // This funcion copies data stored in |buffer_| to |shared_writer_| and
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
  // True if OnWillRead was deferred, in order to wait to be able to allocate a
  // buffer.
  bool did_defer_on_will_read_ = false;
  bool did_defer_on_writing_ = false;
  bool first_read_ = true;

  // Pointer to parent's information about the read buffer. Only non-null while
  // OnWillRead is deferred.
  scoped_refptr<net::IOBuffer>* parent_buffer_ = nullptr;
  int* parent_buffer_size_ = nullptr;
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
