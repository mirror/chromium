// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/webpackage_response_handler.h"

#include "base/bind.h"
#include "base/callback_helpers.h"
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

class WebPackageResponseHandler::BufferedStream : public net::SourceStream {
 public:
  explicit BufferedStream(const GURL& url)
      : SourceStream(SourceStream::TYPE_NONE), url_(url) {}
  ~BufferedStream() override {}
  int Read(net::IOBuffer* dest_buffer,
           int buffer_size,
           const net::CompletionCallback& callback) override {
    LOG(ERROR) << "  BufferedStream::Read " << buffer_size << " " << url_;
    LOG(ERROR) << "  BufferedStream::Read read_buffers_.size() "
               << read_buffers_.size() << " " << url_;
    if (read_buffers_.empty()) {
      if (finished_)
        return 0;
      pending_dest_buffer_ = dest_buffer;
      pending_buffer_size_ = buffer_size;
      pending_read_callback_ = callback;
      return net::ERR_IO_PENDING;
    }
    int written_size = 0;
    while (written_size < buffer_size && !read_buffers_.empty()) {
      auto& src = read_buffers_.front();
      size_t size =
          std::min(base::checked_cast<size_t>(src->BytesRemaining()),
                   base::checked_cast<size_t>(buffer_size - written_size));
      memcpy(dest_buffer->data() + written_size, src->data(), size);
      src->DidConsume(size);
      written_size += size;
      if (src->BytesRemaining() == 0)
        read_buffers_.pop_front();
    }
    LOG(ERROR) << "   BufferedStream::Read written_size: " << written_size
               << " " << url_;
    return written_size;
  }

  std::string Description() const override { return "bufferd stream"; }

  void CopyRemainingTemporaryBuffer() {
    LOG(ERROR) << "BufferedStream::CopyRemainingTemporaryBuffer " << url_;
    if (read_buffers_.empty()) {
      LOG(ERROR) << "  Already consumed " << url_;
      // Already consumed.
      return;
    }
    scoped_refptr<net::DrainableIOBuffer> last_buffer = read_buffers_.back();
    read_buffers_.pop_back();
    auto buffer =
        base::MakeRefCounted<net::IOBuffer>(last_buffer->BytesRemaining());
    read_buffers_.push_back(base::MakeRefCounted<net::DrainableIOBuffer>(
        buffer.get(), last_buffer->BytesRemaining()));
    memcpy(buffer->data(), last_buffer->data(), last_buffer->BytesRemaining());
  }

  bool Empty() const { return read_buffers_.empty(); }

  base::ScopedClosureRunner OnDataReceived(const char* data, size_t size) {
    LOG(ERROR) << "BufferedStream::OnDataReceived " << size << " " << url_;
    auto buffer = base::MakeRefCounted<net::WrappedIOBuffer>(data);
    read_buffers_.push_back(
        base::MakeRefCounted<net::DrainableIOBuffer>(buffer.get(), size));
    LOG(ERROR) << "BufferedStream::OnDataReceived read_buffers_.size: "
               << read_buffers_.size() << " " << url_;

    return base::ScopedClosureRunner(base::Bind(
        &BufferedStream::CopyRemainingTemporaryBuffer, base::Unretained(this)));
  }

  void NotifyFinished() {
    LOG(ERROR) << "BufferedStream::NotifyFinished " << url_;
    finished_ = true;
  }

  void RunCallback() {
    LOG(ERROR) << "BufferedStream::RunCallback " << url_;
    CHECK(pending_dest_buffer_);
    if (read_buffers_.empty()) {
      CHECK(finished_);
      pending_dest_buffer_ = nullptr;
      pending_buffer_size_ = 0;
      base::ResetAndReturn(&pending_read_callback_).Run(0);
      return;
    }
    CHECK(read_buffers_.size() == 1);
    auto& src = read_buffers_.front();
    size_t size = std::min(base::checked_cast<size_t>(src->BytesRemaining()),
                           base::checked_cast<size_t>(pending_buffer_size_));
    memcpy(pending_dest_buffer_->data(), src->data(), size);
    src->DidConsume(size);
    if (src->BytesRemaining() == 0)
      read_buffers_.pop_front();
    pending_dest_buffer_ = nullptr;
    pending_buffer_size_ = 0;
    base::ResetAndReturn(&pending_read_callback_).Run(size);
  }

 private:
  const GURL& url_;
  // For buffering the package data returned by the reader.
  std::deque<scoped_refptr<net::DrainableIOBuffer>> read_buffers_;
  bool finished_ = false;

  scoped_refptr<net::IOBuffer> pending_dest_buffer_ = nullptr;
  int pending_buffer_size_ = 0;
  base::Callback<void(int)> pending_read_callback_;
};

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
      done_callback_(std::move(done_callback)),
      on_read_callback_(base::Bind(&WebPackageResponseHandler::OnReadable,
                                   base::Unretained(this))) {
  LOG(ERROR) << "[" << request_url_ << "] Created ResponseHandler.";

  source_stream_ = std::make_unique<BufferedStream>(request_url_);
  buffered_stream_ = static_cast<BufferedStream*>(source_stream_.get());

  if (support_filters) {
    std::string type;
    size_t iter = 0;
    while (response_head_.headers->EnumerateHeader(&iter, "Content-Encoding",
                                                   &type)) {
      net::SourceStream::SourceType source_type =
          net::FilterSourceStream::ParseEncodingType(type);
      if (source_type == net::SourceStream::TYPE_DEFLATE ||
          source_type == net::SourceStream::TYPE_GZIP) {
        LOG(ERROR) << "  GzipSourceStream ";
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
  LOG(ERROR) << "WebPackageResponseHandler::RegisterCallbacks " << request_url_;
  DCHECK(!completion_callback_);
  DCHECK(!producer_handle_.is_valid());

  mojo::DataPipe data_pipe(512 * 1024);
  producer_handle_ = std::move(data_pipe.producer_handle);
  writable_handle_watcher_->Watch(
      producer_handle_.get(), MOJO_HANDLE_SIGNAL_WRITABLE,
      base::Bind(&WebPackageResponseHandler::OnPipeWritable,
                 base::Unretained(this)));

  completion_callback_ = std::move(completion_callback);
  std::move(callback).Run(net::OK, response_head_,
                          std::move(data_pipe.consumer_handle));
  writable_handle_watcher_->ArmOrNotify();
}

void WebPackageResponseHandler::OnDataReceived(const void* data, size_t size) {
  LOG(ERROR) << "WebPackageResponseHandler::OnDataReceived " << size << "  "
             << request_url_;
  base::ScopedClosureRunner scoped_guard =
      buffered_stream_->OnDataReceived(static_cast<const char*>(data), size);
  if (pending_write_) {
    buffered_stream_->RunCallback();
  }
}

void WebPackageResponseHandler::OnNotifyFinished(int error_code) {
  LOG(ERROR) << "WebPackageResponseHandler::OnNotifyFinished " << error_code
             << "  " << request_url_;
  if (error_code != net::OK) {
    LOG(ERROR) << "OnNotifyFinished error " << error_code << " "
               << request_url_;
    // TODO: Abort
    return;
  }
  buffered_stream_->NotifyFinished();
  if (pending_write_) {
    buffered_stream_->RunCallback();
  }
}

void WebPackageResponseHandler::OnPipeWritable(MojoResult result) {
  LOG(ERROR) << "WebPackageResponseHandler::OnPipeWritable " << result << "  "
             << request_url_;
  ReadData();
}

void WebPackageResponseHandler::ReadData() {
  LOG(ERROR) << "WebPackageResponseHandler::ReadData " << request_url_;
  if (!connected_to_consumer()) {
    LOG(ERROR) << " not connected " << request_url_;
    return;
  }
  if (pending_write_) {
    LOG(ERROR) << " pending_write " << request_url_;
    return;
  }
  uint32_t mojo_buffer_size = 0;
  MojoResult result = network::NetToMojoPendingBuffer::BeginWrite(
      &producer_handle_, &pending_write_, &mojo_buffer_size);
  if (result == MOJO_RESULT_SHOULD_WAIT) {
    // The pipe is full.
    writable_handle_watcher_->ArmOrNotify();
    return;
  }
  if (result != MOJO_RESULT_OK) {
    LOG(ERROR) << "BeginWrite error " << result << " " << request_url_;
    // TODO: Abort
    return;
  }

  writing_data_size_ = 0;
  pending_buffer_size_ = base::checked_cast<int>(mojo_buffer_size);
  ContinueRead();
}

void WebPackageResponseHandler::OnReadable(int net_err) {
  LOG(ERROR) << "WebPackageResponseHandler::OnReadable " << net_err << " "
             << request_url_;
  if (net_err == net::OK) {
    finished_ = true;
  } else if (net_err < net::OK) {
    LOG(ERROR) << "Stream Read error " << net_err << " " << request_url_;
    // TODO: Abort
    return;
  }
  writing_data_size_ += net_err;
  ContinueRead();
}

void WebPackageResponseHandler::ContinueRead() {
  LOG(ERROR) << "WebPackageResponseHandler::ContinueRead " << request_url_;
  while (writing_data_size_ < pending_buffer_size_ && !finished_) {
    auto dest = base::MakeRefCounted<network::NetToMojoIOBuffer>(
        pending_write_.get(), writing_data_size_);
    LOG(ERROR) << " source_stream_->Read " << request_url_;
    int rv = source_stream_->Read(
        dest.get(),
        base::checked_cast<int>(pending_buffer_size_ - writing_data_size_),
        on_read_callback_);
    LOG(ERROR) << " source_stream_->Read done " << rv << " " << request_url_;
    if (rv == net::ERR_IO_PENDING) {
      return;
    } else if (rv == net::OK) {
      finished_ = true;
    } else if (rv < net::OK) {
      LOG(ERROR) << "Stream Read error " << rv << " " << request_url_;
      // TODO: Abort
      return;
    }
    writing_data_size_ += rv;
  }
  LOG(ERROR) << "WebPackageResponseHandler::ContinueRead Complete "
             << writing_data_size_ << " " << request_url_;
  producer_handle_ = pending_write_->Complete(writing_data_size_);
  writing_data_size_ = 0;
  pending_write_ = nullptr;

  if (finished_) {
    LOG(ERROR) << "finished " << request_url_;
    CompleteAndNotify();
  } else {
    writable_handle_watcher_->ArmOrNotify();
  }
}

void WebPackageResponseHandler::CompleteAndNotify() {
  LOG(ERROR) << "- CompleteAndNotify: [" << request_url_ << "]";
  writable_handle_watcher_.reset();
  producer_handle_.reset();
  if (completion_callback_) {
    std::move(completion_callback_)
        .Run(network::URLLoaderCompletionStatus(net::OK));
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

WebPackagePendingCallbacks& WebPackagePendingCallbacks::operator=(
    WebPackagePendingCallbacks&& other) {
  callback = std::move(other.callback);
  completion_callback = std::move(other.completion_callback);
  return *this;
}

}  // namespace content
