// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/webpackage_response_handler.h"

#include "base/bind.h"
#include "base/debug/stack_trace.h"
#include "base/numerics/safe_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "content/public/common/resource_request.h"
#include "net/base/io_buffer.h"
#include "net/filter/filter_source_stream.h"
#include "net/filter/gzip_source_stream.h"
#include "net/filter/source_stream.h"

namespace content {

class WebPackageResponseHandler::WebPackageSourceStream
    : public net::SourceStream {
 public:
  explicit WebPackageSourceStream(WebPackageResponseHandler* handler)
      : SourceStream(SourceStream::TYPE_NONE), handler_(handler) {}
  ~WebPackageSourceStream() override {}

  int Read(net::IOBuffer* dest_buffer,
           int buffer_size,
           const net::CompletionCallback& callback) override {
    return handler_->Read(dest_buffer, buffer_size);
  }
  std::string Description() const override { return "data pipe"; }

 private:
  // Not owned, handler owns this.
  WebPackageResponseHandler* handler_ = nullptr;
};

//----------------------------------------------------------------

WebPackageResponseHandler::WebPackageResponseHandler(
    const GURL& request_url,
    const std::string& request_method,
    ResourceResponseHead head,
    base::OnceClosure done_callback,
    bool support_filters)
    : request_url_(request_url),
      request_method_(request_method),
      response_head_(std::move(head)),
      writable_handle_watcher_(std::make_unique<mojo::SimpleWatcher>(
          FROM_HERE,
          mojo::SimpleWatcher::ArmingPolicy::MANUAL)),
      done_callback_(std::move(done_callback)) {
  LOG(ERROR) << "[" << request_url_ << "] Created ResponseHandler.";

  source_stream_ = std::make_unique<WebPackageSourceStream>(this);

  if (support_filters) {
    std::string type;
    size_t iter = 0;
    while (response_head_.headers->EnumerateHeader(&iter, "Content-Encoding",
                                                   &type)) {
      net::SourceStream::SourceType source_type =
          net::FilterSourceStream::ParseEncodingType(type);
      if (source_type == net::SourceStream::TYPE_DEFLATE ||
          source_type == net::SourceStream::TYPE_GZIP) {
        source_stream_ = net::GzipSourceStream::Create(
            std::move(source_stream_), source_type);
      } else {
        // TODO: Maybe we could possibly support TYPE_BROTLI etc.
        NOTREACHED();
      }
    }
  }
}

WebPackageResponseHandler::~WebPackageResponseHandler() {}

void WebPackageResponseHandler::RegisterCallbacks(
    ResourceCallback callback,
    CompletionCallback completion_callback) {
  DCHECK(!completion_callback_);
  DCHECK(!producer_handle_.is_valid());

  mojo::DataPipe data_pipe(512 * 1024);
  producer_handle_ = std::move(data_pipe.producer_handle);
  writable_handle_watcher_->Watch(
      producer_handle_.get(), MOJO_HANDLE_SIGNAL_WRITABLE,
      base::Bind(&WebPackageResponseHandler::OnPipeWritable,
                 base::Unretained(this)));
  if (!read_buffers_.empty()) {
    writable_handle_watcher_->ArmOrNotify();
  }

  completion_callback_ = std::move(completion_callback);
  std::move(callback).Run(net::OK, response_head_,
                          std::move(data_pipe.consumer_handle));
}

void WebPackageResponseHandler::PushDataToBuffer(const void* data,
                                                 size_t size) {
  DCHECK(data);
  auto buffer = base::MakeRefCounted<net::IOBuffer>(size);
  read_buffers_.push_back(
      base::MakeRefCounted<net::DrainableIOBuffer>(buffer.get(), size));
  memcpy(buffer->data(), static_cast<const char*>(data), size);

  if (completion_callback_)
    writable_handle_watcher_->ArmOrNotify();
}

void WebPackageResponseHandler::OnDataReceived(const void* data, size_t size) {
  if (!read_buffers_.empty() || !completion_callback_) {
    PushDataToBuffer(data, size);
    return;
  }
  DCHECK(completion_callback_);

  uint32_t available = 0;
  scoped_refptr<network::NetToMojoPendingBuffer> pending_write;
  MojoResult result = network::NetToMojoPendingBuffer::BeginWrite(
      &producer_handle_, &pending_write, &available);
  if (result == MOJO_RESULT_SHOULD_WAIT || available == 0) {
    // The pipe is full.
    PushDataToBuffer(data, size);
    return;
  }
  if (result != MOJO_RESULT_OK) {
    LOG(ERROR) << "BeginWrite error " << result;
    Abort(net::ERR_FAILED);
    return;
  }
  DCHECK(!producer_handle_.is_valid());

  size_t written_size = 0;
  size_t consumed_size = 0;
  int rv = WriteDataToPipe(pending_write.get(), 0,
                           base::checked_cast<size_t>(available), data, size,
                           &written_size, &consumed_size);
  if (rv < net::OK) {
    LOG(ERROR) << "WriteDataToPipe error " << rv;
    Abort(rv);
    return;
  }
  producer_handle_ = pending_write->Complete(written_size);

  DCHECK_LE(consumed_size, size);

  if (size != consumed_size) {
    PushDataToBuffer(static_cast<const char*>(data) + consumed_size,
                     size - consumed_size);
  }
}

void WebPackageResponseHandler::OnNotifyFinished(int error_code) {
  error_code_ = error_code;
  source_finished_ = true;
  if (completion_callback_) {
    if (!read_buffers_.empty() || stream_need_to_be_drained_) {
      WriteBufferToPipe();
    } else {
      CompleteAndNotify();
    }
  }
}

void WebPackageResponseHandler::OnPipeWritable(MojoResult mojo_result) {
  if (mojo_result == MOJO_RESULT_FAILED_PRECONDITION) {
    LOG(ERROR) << "Canceled? " << request_url_;
    Abort(net::ERR_ABORTED);
    return;
  }
  DCHECK_EQ(mojo_result, MOJO_RESULT_OK) << request_url_;
  WriteBufferToPipe();
}

void WebPackageResponseHandler::WriteBufferToPipe() {
  uint32_t available = 0;
  scoped_refptr<network::NetToMojoPendingBuffer> pending_write;
  MojoResult result = network::NetToMojoPendingBuffer::BeginWrite(
      &producer_handle_, &pending_write, &available);
  if (result == MOJO_RESULT_SHOULD_WAIT) {
    writable_handle_watcher_->ArmOrNotify();
    return;
  }
  if (result != MOJO_RESULT_OK) {
    LOG(ERROR) << "BeginWrite error " << result << " url " << request_url_;
    Abort(net::ERR_FAILED);
    return;
  }
  DCHECK(!producer_handle_.is_valid());
  const size_t dest_buffer_size = base::checked_cast<size_t>(available);

  size_t total_written_size = 0;

  // Read from |read_buffers_|.
  while (!read_buffers_.empty() && dest_buffer_size - total_written_size > 0) {
    auto& src = read_buffers_.front();
    size_t written_size = 0;
    size_t consumed_size = 0;
    int ret =
        WriteDataToPipe(pending_write.get(), total_written_size,
                        dest_buffer_size - total_written_size, src->data(),
                        base::checked_cast<size_t>(src->BytesRemaining()),
                        &written_size, &consumed_size);
    if (ret < net::OK) {
      LOG(ERROR) << "WriteDataToPipe error " << ret << "  " << request_url_;
      Abort(ret);
      return;
    }
    src->DidConsume(consumed_size);
    total_written_size += written_size;
    if (src->BytesRemaining() > 0)
      break;
    read_buffers_.pop_front();
  }

  // Read from the buffer in |source_stream_|.
  if (read_buffers_.empty() && source_finished_ && stream_need_to_be_drained_ &&
      dest_buffer_size - total_written_size > 0) {
    size_t written_size = 0;
    bool finished = false;
    int ret = DrainDataToPipe(pending_write.get(), total_written_size,
                              dest_buffer_size - total_written_size,
                              &written_size, &finished);
    if (ret < net::OK) {
      LOG(ERROR) << "DrainDataToPipe error " << ret << "  " << request_url_;
      Abort(ret);
      return;
    }
    total_written_size += written_size;
    if (finished) {
      stream_need_to_be_drained_ = false;
    }
  }
  producer_handle_ = pending_write->Complete(total_written_size);

  if (!read_buffers_.empty() || stream_need_to_be_drained_) {
    if (total_written_size)
      writable_handle_watcher_->ArmOrNotify();
  } else if (source_finished_) {
    CompleteAndNotify();
  }
}

int WebPackageResponseHandler::Read(net::IOBuffer* dest_buffer,
                                    int buffer_size) {
  if (!current_read_pointer_) {
    return 0;
  }
  size_t size =
      std::min(current_read_size_, base::checked_cast<size_t>(buffer_size));
  memcpy(dest_buffer->data(), current_read_pointer_, size);
  consumed_read_size_ = size;
  return size;
}

int WebPackageResponseHandler::WriteDataToPipe(
    network::NetToMojoPendingBuffer* dest_buffer,
    size_t dest_offset,
    size_t dest_size,
    const void* src,
    size_t src_size,
    size_t* written_size,
    size_t* consumed_size) {
  DCHECK(!current_read_pointer_);
  DCHECK(src_size);
  stream_need_to_be_drained_ = true;
  size_t read = 0, written = 0;
  while (dest_size > written && src_size > read) {
    current_read_pointer_ = static_cast<const char*>(src) + read;
    current_read_size_ = src_size - read;
    consumed_read_size_ = 0;
    auto dest = base::MakeRefCounted<network::NetToMojoIOBuffer>(
        dest_buffer, base::checked_cast<int>(dest_offset + written));
    int rv = source_stream_->Read(dest.get(),
                                  base::checked_cast<int>(dest_size - written),
                                  net::CompletionCallback());
    current_read_pointer_ = nullptr;
    DCHECK(rv != net::OK);
    if (rv < net::OK)
      return rv;
    read += consumed_read_size_;
    written += rv;
  }
  *written_size = written;
  *consumed_size = read;
  return net::OK;
}

int WebPackageResponseHandler::DrainDataToPipe(
    network::NetToMojoPendingBuffer* dest_buffer,
    size_t dest_offset,
    size_t dest_size,
    size_t* written_size,
    bool* finished) {
  *finished = false;
  size_t written = 0;
  while (dest_size > written) {
    auto dest = base::MakeRefCounted<network::NetToMojoIOBuffer>(
        dest_buffer, base::checked_cast<int>(dest_offset + written));
    int rv = source_stream_->Read(dest.get(),
                                  base::checked_cast<int>(dest_size - written),
                                  net::CompletionCallback());
    if (rv == net::OK) {
      *finished = true;
      break;
    }
    if (rv < net::OK)
      return rv;
    written += rv;
  }
  *written_size = written;
  return net::OK;
}

void WebPackageResponseHandler::CompleteAndNotify() {
  LOG(ERROR) << "- CompleteAndNotify: [" << request_url_ << "]";
  writable_handle_watcher_.reset();
  producer_handle_.reset();
  if (completion_callback_) {
    std::move(completion_callback_)
        .Run(network::URLLoaderCompletionStatus(error_code_));
  }
  // May delete this.
  std::move(done_callback_).Run();
}

void WebPackageResponseHandler::Abort(int error_code) {
  error_code_ = error_code;
  CompleteAndNotify();
}

//--------------------------------------------------------------------

WebPackagePendingCallbacks::WebPackagePendingCallbacks(
    ResourceCallback callback,
    CompletionCallback completion_callback)
    : callback(std::move(callback)),
      completion_callback(std::move(completion_callback)) {}

WebPackagePendingCallbacks::~WebPackagePendingCallbacks() = default;

WebPackagePendingCallbacks::WebPackagePendingCallbacks(
    WebPackagePendingCallbacks&& other) {
  *this = std::move(other);
}

WebPackagePendingCallbacks& WebPackagePendingCallbacks::operator=(
    WebPackagePendingCallbacks&& other) {
  callback = std::move(other.callback);
  completion_callback = std::move(other.completion_callback);
  return *this;
}

}  // namespace content
