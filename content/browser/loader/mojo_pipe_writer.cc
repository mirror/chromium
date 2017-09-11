// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/mojo_pipe_writer.h"

#include "base/bind.h"
#include "base/command_line.h"
#include "base/location.h"
#include "base/memory/ptr_util.h"
#include "base/strings/string_number_conversions.h"
#include "mojo/public/c/system/data_pipe.h"
#include "mojo/public/cpp/bindings/message.h"
#include "net/base/mime_sniffer.h"
#include "net/base/net_errors.h"

namespace content {
namespace {

int g_allocation_size = MojoPipeWriter::kDefaultAllocationSize;

// MimeTypeResourceHandler *implicitly* requires that the buffer size
// returned from InitializeDataBuffer should be larger than certain size.
// TODO(yhirano): Fix MimeTypeResourceHandler.
constexpr size_t kMinAllocationSize = 2 * net::kMaxBytesToSniff;

constexpr size_t kMaxChunkSize = 32 * 1024;

void GetNumericArg(const std::string& name, int* result) {
  const std::string& value =
      base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(name);
  if (!value.empty())
    base::StringToInt(value, result);
}

void InitializeResourceBufferConstants() {
  static bool did_init = false;
  if (did_init)
    return;
  did_init = true;

  GetNumericArg("resource-buffer-size", &g_allocation_size);
}

}  // namespace

// This class is for sharing the ownership of a ScopedDataPipeProducerHandle
// between WriterIOBuffer and MojoAsyncResourceHandler.
class MojoPipeWriter::SharedWriter final
    : public base::RefCountedThreadSafe<SharedWriter> {
 public:
  explicit SharedWriter(mojo::ScopedDataPipeProducerHandle writer)
      : writer_(std::move(writer)) {}
  mojo::DataPipeProducerHandle writer() { return writer_.get(); }

 private:
  friend class base::RefCountedThreadSafe<SharedWriter>;
  ~SharedWriter() {}

  const mojo::ScopedDataPipeProducerHandle writer_;

  DISALLOW_COPY_AND_ASSIGN(SharedWriter);
};

// This class is a IOBuffer subclass for data gotten from a
// ScopedDataPipeProducerHandle.
class MojoPipeWriter::WriterIOBuffer final : public net::IOBufferWithSize {
 public:
  // |data| and |size| should be gotten from |writer| via BeginWriteData.
  // They will be accessible via IOBuffer methods. As |writer| is stored in this
  // instance, |data| will be kept valid as long as the following conditions
  // hold:
  //  1. |data| is not invalidated via EndWriteDataRaw.
  //  2. |this| instance is alive.
  WriterIOBuffer(scoped_refptr<SharedWriter> writer, void* data, size_t size)
      : net::IOBufferWithSize(static_cast<char*>(data), size),
        writer_(std::move(writer)) {}

 private:
  ~WriterIOBuffer() override {
    // Avoid deleting |data_| in the IOBuffer destructor.
    data_ = nullptr;
  }

  // This member is for keeping the writer alive.
  scoped_refptr<SharedWriter> writer_;

  DISALLOW_COPY_AND_ASSIGN(WriterIOBuffer);
};

MojoPipeWriter::MojoPipeWriter(
    MojoPipeWriter::Delegate* delegate,
    mojo::ScopedDataPipeConsumerHandle* consumer_handle)
    : delegate_(delegate),
      handle_watcher_(FROM_HERE, mojo::SimpleWatcher::ArmingPolicy::MANUAL),
      consumer_handle_(consumer_handle),
      weak_factory_(this) {
  InitializeResourceBufferConstants();
}

MojoPipeWriter::~MojoPipeWriter() {}

net::Error MojoPipeWriter::InitializeDataBuffer(scoped_refptr<net::IOBuffer>* buf,
                                      int* buf_size) {
  // |buffer_| is set to nullptr on successful read completion (Except for the
  // final 0-byte read, so this DCHECK will also catch InitializeDataBuffer being called
  // after WriteFromDataBuffer(0)).
  DCHECK(!buffer_);
  DCHECK_EQ(0u, buffer_offset_);

  bool first_call = first_write_;
  first_write_ = false;

  net::Error result = InitializeDataPipeIfNeeded();
  if (result != net::OK)
    return result;

  DCHECK(shared_writer_);

  bool defer = false;
  if (!AllocateWriterIOBuffer(&buffer_, &defer))
    return net::ERR_INSUFFICIENT_RESOURCES;

  if (defer) {
    DCHECK(!buffer_);
    provided_buffer_ = buf;
    provided_buffer_size_ = buf_size;
    did_defer_on_initializing_buffer_ = true;
    return net::ERR_IO_PENDING;
  }

  // The first call to InitializeDataBuffer must return a buffer of at least
  // kMinAllocationSize. If the Mojo buffer is too small, need to allocate an
  // intermediary buffer.
  if (first_call && static_cast<size_t>(buffer_->size()) < kMinAllocationSize) {
    // The allocated buffer is too small, so need to create an intermediary one.
    if (EndWrite(0) != MOJO_RESULT_OK)
      return net::ERR_INSUFFICIENT_RESOURCES;

    DCHECK(!is_using_io_buffer_not_from_writer_);
    is_using_io_buffer_not_from_writer_ = true;
    buffer_ = new net::IOBufferWithSize(kMinAllocationSize);
  }

  *buf = buffer_;
  *buf_size = buffer_->size();
  return net::OK;
}

net::Error MojoPipeWriter::WriteFromDataBuffer(int bytes_read) {
  DCHECK_GE(bytes_read, 0);
  DCHECK(buffer_);

  if (bytes_read == 0) {
    // Note that |buffer_| is not cleared here, which will cause a DCHECK on
    // subsequent InitializeDataBuffer calls.
    return net::OK;
  }

  if (is_using_io_buffer_not_from_writer_) {
    // Couldn't allocate a large enough buffer on the data pipe in InitializeDataBuffer.
    DCHECK_EQ(0u, buffer_bytes_read_);
    buffer_bytes_read_ = bytes_read;
    bool defer = false;
    if (!CopyReadDataToDataPipe(&defer))
      return net::ERR_INSUFFICIENT_RESOURCES;

    if (defer) {
      did_defer_on_writing_ = true;
      return net::ERR_IO_PENDING;
    }

    return net::OK;
  }

  if (EndWrite(bytes_read) != MOJO_RESULT_OK) {
    return net::ERR_ABORTED;
  }

  buffer_ = nullptr;
  return net::OK;
}

net::Error MojoPipeWriter::InitializeDataPipeIfNeeded() {
  if (shared_writer_)
    return net::OK;

  MojoCreateDataPipeOptions options;
  options.struct_size = sizeof(MojoCreateDataPipeOptions);
  options.flags = MOJO_CREATE_DATA_PIPE_OPTIONS_FLAG_NONE;
  options.element_num_bytes = 1;
  options.capacity_num_bytes = g_allocation_size;
  mojo::ScopedDataPipeProducerHandle producer;

  MojoResult result =
      mojo::CreateDataPipe(&options, &producer, consumer_handle_);

  if (result != MOJO_RESULT_OK)
    return net::ERR_INSUFFICIENT_RESOURCES;

  shared_writer_ = new SharedWriter(std::move(producer));
  handle_watcher_.Watch(
      shared_writer_->writer(), MOJO_HANDLE_SIGNAL_WRITABLE,
      base::Bind(&MojoPipeWriter::OnWritable, base::Unretained(this)));
  handle_watcher_.ArmOrNotify();
  return net::OK;
}

void MojoPipeWriter::Finalize() {
  shared_writer_ = nullptr;
  buffer_ = nullptr;
  handle_watcher_.Cancel();
}

void MojoPipeWriter::OnWritableForTesting() {
  OnWritable(MOJO_RESULT_OK);
}

void MojoPipeWriter::SetAllocationSizeForTesting(size_t size) {
  g_allocation_size = size;
}

MojoResult MojoPipeWriter::BeginWrite(void** data, uint32_t* available) {
  MojoResult result = shared_writer_->writer().BeginWriteData(
      data, available, MOJO_WRITE_DATA_FLAG_NONE);
  if (result == MOJO_RESULT_OK)
    *available = std::min(*available, static_cast<uint32_t>(kMaxChunkSize));
  else if (result == MOJO_RESULT_SHOULD_WAIT)
    handle_watcher_.ArmOrNotify();
  return result;
}

MojoResult MojoPipeWriter::EndWrite(uint32_t written) {
  MojoResult result = shared_writer_->writer().EndWriteData(written);
  if (result == MOJO_RESULT_OK) {
    total_written_bytes_ += written;
    handle_watcher_.ArmOrNotify();
  }
  return result;
}

bool MojoPipeWriter::CopyReadDataToDataPipe(bool* defer) {
  while (buffer_bytes_read_ > 0) {
    scoped_refptr<net::IOBufferWithSize> dest;
    if (!AllocateWriterIOBuffer(&dest, defer))
      return false;
    if (*defer)
      return true;

    size_t copied_size =
        std::min(buffer_bytes_read_, static_cast<size_t>(dest->size()));
    memcpy(dest->data(), buffer_->data() + buffer_offset_, copied_size);
    buffer_offset_ += copied_size;
    buffer_bytes_read_ -= copied_size;
    if (EndWrite(copied_size) != MOJO_RESULT_OK)
      return false;
  }

  // All bytes are copied.
  buffer_ = nullptr;
  buffer_offset_ = 0;
  is_using_io_buffer_not_from_writer_ = false;
  return true;
}

bool MojoPipeWriter::AllocateWriterIOBuffer(
    scoped_refptr<net::IOBufferWithSize>* buf,
    bool* defer) {
  void* data = nullptr;
  uint32_t available = 0;
  MojoResult result = BeginWrite(&data, &available);
  if (result == MOJO_RESULT_SHOULD_WAIT) {
    *defer = true;
    return true;
  }
  if (result != MOJO_RESULT_OK)
    return false;
  DCHECK_GT(available, 0u);
  *buf = new WriterIOBuffer(shared_writer_, data, available);
  return true;
}

void MojoPipeWriter::OnWritable(MojoResult result) {
  DCHECK(!did_defer_on_initializing_buffer_ || !did_defer_on_writing_);
  if (did_defer_on_initializing_buffer_) {
    did_defer_on_initializing_buffer_ = false;

    scoped_refptr<net::IOBuffer>* provided_buffer = provided_buffer_;
    provided_buffer_ = nullptr;
    int* provided_buffer_size = provided_buffer_size_;
    provided_buffer_size_ = nullptr;

    net::Error result = InitializeDataBuffer(provided_buffer, provided_buffer_size);
    if (result != net::ERR_IO_PENDING)
      delegate_->OnPipeWriterOperationComplete(result);
    return;
  }

  if (!did_defer_on_writing_)
    return;

  did_defer_on_writing_ = false;

  DCHECK(is_using_io_buffer_not_from_writer_);
  // |buffer_| is set to a net::IOBufferWithSize. Write the buffer contents
  // to the data pipe.
  DCHECK_GT(buffer_bytes_read_, 0u);
  if (!CopyReadDataToDataPipe(&did_defer_on_writing_)) {
    delegate_->OnPipeWriterOperationComplete(net::ERR_INSUFFICIENT_RESOURCES);
    return;
  }

  if (did_defer_on_writing_) {
    // Continue waiting.
    return;
  }
  delegate_->OnPipeWriterOperationComplete(net::OK);
}

}  // namespace content
