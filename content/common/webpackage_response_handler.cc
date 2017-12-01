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

class WebPackageSourceStream : public net::SourceStream {
 public:
  explicit WebPackageSourceStream(WebPackageResponseHandler* handler)
      : SourceStream(SourceStream::TYPE_NONE), handler_(handler) {}
  ~WebPackageSourceStream() override {}

  int Read(net::IOBuffer* dest_buffer,
           int buffer_size,
           const net::CompletionCallback& callback) override {
    const char* src_buffer = nullptr;
    size_t src_size = 0;
    handler_->GetCurrentReadBuffer(&src_buffer, &src_size);
    size_t size = std::min(src_size, base::checked_cast<size_t>(buffer_size));
    memcpy(dest_buffer->data(), src_buffer, size);
    handler_->UpdateConsumedReadSize(size);
    return size;
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
  // LOG(ERROR) << "[" << request_url_ << "] Created ResponseHandler.";

  source_stream_ = std::make_unique<WebPackageSourceStream>(this);

  if (support_filters) {
    std::string type;
    size_t iter = 0;
    while (response_head_.headers->EnumerateHeader(
            &iter, "Content-Encoding", &type)) {
      net::SourceStream::SourceType source_type =
          net::FilterSourceStream::ParseEncodingType(type);
      if (source_type == net::SourceStream::TYPE_DEFLATE ||
          source_type == net::SourceStream::TYPE_GZIP) {
        source_stream_ =
            net::GzipSourceStream::Create(std::move(source_stream_),
                                          source_type);
      } else {
        // TODO: Maybe we could possibly support TYPE_BROTLI etc.
        NOTREACHED();
      }
    }
  }
}

WebPackageResponseHandler::~WebPackageResponseHandler() = default;

void WebPackageResponseHandler::AddCallbacks(
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
  if (!read_buffers_.empty())
    writable_handle_watcher_->ArmOrNotify();

  completion_callback_ = std::move(completion_callback);
  std::move(callback).Run(net::OK, response_head_,
                          std::move(data_pipe.consumer_handle));
}

void WebPackageResponseHandler::OnDataReceived(const void* data, size_t size) {
  size_t consumed = 0;
  // LOG(ERROR) << "[" << request_url_ << "] OnDataReceived:" << size;

  if (producer_handle_.is_valid()) {
    uint32_t available = 0;
    scoped_refptr<network::NetToMojoPendingBuffer> pending_write;
    auto result = BeginWriteToPipe(&pending_write, &available);
    if (available > 0u && !data_write_finished_) {
      DCHECK_EQ(MOJO_RESULT_OK, result);
      size_t dest_size = base::checked_cast<size_t>(available);
      consumed = size;
      WriteToPipe(pending_write.get(), 0, &dest_size, data, &consumed);
      // LOG(ERROR) << "- consumed / written to the pipe: " << size;
      producer_handle_ = pending_write->Complete(dest_size);
    }
    DCHECK(producer_handle_.is_valid());
  }

  // Drop data if the pipe is closed.
  if (data_write_finished_)
    return;

  CHECK_LE(consumed, size);
  size_t remaining = size - consumed;
  if (remaining == 0)
    return;

  auto buffer = base::MakeRefCounted<net::IOBuffer>(remaining);
  read_buffers_.push_back(
      base::MakeRefCounted<net::DrainableIOBuffer>(buffer.get(), remaining));
  memcpy(buffer->data(), static_cast<const char*>(data) + consumed, remaining);
  // LOG(ERROR) << "- saving to the buffer: " << remaining;

  if (producer_handle_.is_valid())
    writable_handle_watcher_->ArmOrNotify();
}

void WebPackageResponseHandler::OnNotifyFinished(int error_code) {
  // LOG(ERROR) << "[" << request_url_ << "] OnNotifyFinished - data finished.";
  if (error_code_ == net::OK)
    error_code_ = error_code;
  data_receive_finished_ = true;
  if (read_buffers_.empty() && !source_stream_may_have_data_)
    data_write_finished_ = true;
  MaybeCompleteAndNotify();
}

void WebPackageResponseHandler::GetCurrentReadBuffer(const char** buf,
                                                     size_t* size) {
  DCHECK(current_read_pointer_ || current_read_size_ == 0);
  *buf = current_read_pointer_;
  *size = current_read_size_;
}

void WebPackageResponseHandler::UpdateConsumedReadSize(size_t size) {
  consumed_read_size_ = size;
}

void WebPackageResponseHandler::OnPipeWritable(MojoResult mojo_result) {
  if (mojo_result == MOJO_RESULT_FAILED_PRECONDITION) {
    LOG(ERROR) << "Canceled? " << request_url_;
    OnNotifyFinished(net::ERR_ABORTED);
    return;
  }

  DCHECK_EQ(mojo_result, MOJO_RESULT_OK) << request_url_;
  DCHECK(producer_handle_.is_valid());
  DCHECK(!data_write_finished_);
  if (read_buffers_.empty() && !source_stream_may_have_data_)
    return;

  uint32_t available = 0;
  scoped_refptr<network::NetToMojoPendingBuffer> pending_write;
  auto result = BeginWriteToPipe(&pending_write, &available);
  if (available == 0u || data_write_finished_)
    return;
  DCHECK_EQ(MOJO_RESULT_OK, result);

  size_t dest_offset = 0;
  size_t dest_buffer_size = base::checked_cast<size_t>(available);
  while (!read_buffers_.empty() && dest_buffer_size - dest_offset > 0) {
    auto& src = read_buffers_.front();
    size_t src_size = base::checked_cast<size_t>(src->BytesRemaining());
    size_t dest_size = dest_buffer_size - dest_offset;
    int ret = WriteToPipe(pending_write.get(), dest_offset, &dest_size,
                          src->data(), &src_size);
    if (ret != net::OK) {
      LOG(ERROR) << "WriteToPipe error " << ret << "  " << request_url_;
      return;
    }
    src->DidConsume(src_size);
    dest_offset += dest_size;
    if (src->BytesRemaining() > 0)
      break;
    read_buffers_.pop_front();
  }

  size_t dest_size = dest_buffer_size - dest_offset;
  if (read_buffers_.empty() && dest_size > 0 && source_stream_may_have_data_) {
    size_t src_size = 0;
    int ret = WriteToPipe(pending_write.get(), dest_offset, &dest_size, nullptr,
                          &src_size);
    if (ret != net::OK) {
      LOG(ERROR) << "WriteToPipe error " << ret << "  " << request_url_;
      OnNotifyFinished(ret);
      return;
    }
    if (dest_size == 0) {
      source_stream_may_have_data_ = false;
    }

    dest_offset += dest_size;
  }

  producer_handle_ = pending_write->Complete(dest_offset);

  if (read_buffers_.empty() && data_receive_finished_ &&
      !source_stream_may_have_data_) {
    data_write_finished_ = true;
    MaybeCompleteAndNotify();
    return;
  }

  writable_handle_watcher_->ArmOrNotify();
}

MojoResult WebPackageResponseHandler::BeginWriteToPipe(
    scoped_refptr<network::NetToMojoPendingBuffer>* pending_buffer,
    uint32_t* available) {
  MojoResult result = network::NetToMojoPendingBuffer::BeginWrite(
      &producer_handle_, pending_buffer, available);
  if (result == MOJO_RESULT_SHOULD_WAIT) {
    // The pipe is full.
    writable_handle_watcher_->ArmOrNotify();
  }
  if (result != MOJO_RESULT_OK) {
    data_write_finished_ = true;
    writable_handle_watcher_->Cancel();
    producer_handle_.reset();
    error_code_ = net::ERR_FAILED;
    LOG(ERROR) << "ERR_FAILED";
    MaybeCompleteAndNotify();
  }
  return result;
}

int WebPackageResponseHandler::WriteToPipe(
    network::NetToMojoPendingBuffer* dest_buffer,
    size_t dest_offset,
    size_t* dest_size_inout,
    const void* src,
    size_t* src_size_inout) {
  DCHECK(!current_read_pointer_);

  // TODO(kinuko):
  // Read/write buffer handling is a bit ugly here, PendingBuffer should
  // probably support changing the offset like DrainableIOBuffer.
  size_t dest_size = *dest_size_inout;
  size_t src_size = *src_size_inout;
  size_t read = 0, written = 0;
  while (dest_size > written &&
         (src_size > read || (source_stream_may_have_data_ && src_size == 0))) {
    current_read_pointer_ = static_cast<const char*>(src) + read;
    current_read_size_ = src_size - read;
    consumed_read_size_ = 0;
    auto dest = base::MakeRefCounted<network::NetToMojoIOBuffer>(
        dest_buffer, base::checked_cast<int>(dest_offset));
    int rv = source_stream_->Read(dest.get(),
                                  base::checked_cast<int>(dest_size - written),
                                  net::CompletionCallback());
    current_read_pointer_ = nullptr;
    if (rv < 0) {
      data_write_finished_ = true;
      writable_handle_watcher_->Cancel();
      producer_handle_.reset();
      return rv;
    }
    if (rv == net::OK) {
      source_stream_may_have_data_ = false;
      break;
    }
    source_stream_may_have_data_ = true;
    read += consumed_read_size_;
    written += rv;
    dest_offset += rv;
  }
  *dest_size_inout = written;
  *src_size_inout = read;
  return net::OK;
}

void WebPackageResponseHandler::MaybeCompleteAndNotify() {
  if (!data_write_finished_ || !data_receive_finished_)
    return;
  LOG(ERROR) << "- Finished: " << request_url_;
  writable_handle_watcher_.reset();
  producer_handle_.reset();
  if (completion_callback_) {
    std::move(completion_callback_)
        .Run(network::URLLoaderCompletionStatus(error_code_));
  }
  // May delete this.
  std::move(done_callback_).Run();
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

WebPackagePendingCallbacks&
WebPackagePendingCallbacks::operator=(WebPackagePendingCallbacks&& other) {
  callback = std::move(other.callback);
  completion_callback = std::move(other.completion_callback);
  return *this;
}

}  // namespace content
