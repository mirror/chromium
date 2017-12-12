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
#include "base/trace_event/trace_event.h"
#include "base/trace_event/trace_event_argument.h"
#include "content/public/common/resource_request.h"
#include "net/base/io_buffer.h"
#include "net/filter/filter_source_stream.h"
#include "net/filter/gzip_source_stream.h"
#include "net/filter/source_stream.h"

namespace content {

namespace {
const char kTraceEventCategory[] = TRACE_DISABLED_BY_DEFAULT("net");
}  // namespace

class WebPackageResponseHandler::BufferedStream : public net::SourceStream {
 public:
  explicit BufferedStream(void* trace_id)
      : SourceStream(SourceStream::TYPE_NONE), trace_id_(trace_id) {}
  ~BufferedStream() override {}
  int Read(net::IOBuffer* dest_buffer,
           int buffer_size,
           const net::CompletionCallback& callback) override {
    static const char* kTraceEventName = "BufferedStream::Read";
    TRACE_EVENT_NESTABLE_ASYNC_BEGIN1(kTraceEventCategory, kTraceEventName,
                                      TRACE_ID_LOCAL(trace_id_), "buffer_size",
                                      buffer_size);
    if (read_buffers_.empty()) {
      if (finished_) {
        TRACE_EVENT_NESTABLE_ASYNC_END1(kTraceEventCategory, kTraceEventName,
                                        TRACE_ID_LOCAL(trace_id_), "status",
                                        "finished");

        return 0;
      }
      pending_dest_buffer_ = dest_buffer;
      pending_buffer_size_ = buffer_size;
      pending_read_callback_ = callback;
      TRACE_EVENT_NESTABLE_ASYNC_END1(kTraceEventCategory, kTraceEventName,
                                      TRACE_ID_LOCAL(trace_id_), "status",
                                      "pending");
      return net::ERR_IO_PENDING;
    }
    int written_size = 0;
    while (written_size < buffer_size && !read_buffers_.empty()) {
      TRACE_EVENT_NESTABLE_ASYNC_BEGIN0(kTraceEventCategory,
                                        "BufferedStream::Read memcpy",
                                        TRACE_ID_LOCAL(trace_id_));
      auto& src = read_buffers_.front();
      size_t size =
          std::min(base::checked_cast<size_t>(src->BytesRemaining()),
                   base::checked_cast<size_t>(buffer_size - written_size));
      memcpy(dest_buffer->data() + written_size, src->data(), size);
      src->DidConsume(size);
      written_size += size;
      if (src->BytesRemaining() == 0)
        read_buffers_.pop_front();
      TRACE_EVENT_NESTABLE_ASYNC_END1(kTraceEventCategory,
                                      "BufferedStream::Read memcpy",
                                      TRACE_ID_LOCAL(trace_id_), "size", size);
    }
    TRACE_EVENT_NESTABLE_ASYNC_END2(kTraceEventCategory, kTraceEventName,
                                    TRACE_ID_LOCAL(trace_id_), "written_size",
                                    written_size, "remaining_buffer_size",
                                    read_buffers_.size());
    return written_size;
  }

  std::string Description() const override { return "bufferd stream"; }

  void CopyRemainingTemporaryBuffer() {
    static const char* kTraceEventName =
        "BufferedStream::CopyRemainingTemporaryBuffer";
    TRACE_EVENT_NESTABLE_ASYNC_BEGIN0(kTraceEventCategory, kTraceEventName,
                                      TRACE_ID_LOCAL(trace_id_));
    if (read_buffers_.empty()) {
      TRACE_EVENT_NESTABLE_ASYNC_END1(kTraceEventCategory, kTraceEventName,
                                      TRACE_ID_LOCAL(trace_id_), "status",
                                      "Already consumed");

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
    TRACE_EVENT_NESTABLE_ASYNC_END1(kTraceEventCategory, kTraceEventName,
                                    TRACE_ID_LOCAL(trace_id_), "status",
                                    "done");
  }

  bool Empty() const { return read_buffers_.empty(); }

  base::ScopedClosureRunner OnDataReceived(const char* data, size_t size) {
    static const char* kTraceEventName = "BufferedStream::OnDataReceived";
    TRACE_EVENT_NESTABLE_ASYNC_BEGIN1(kTraceEventCategory, kTraceEventName,
                                      TRACE_ID_LOCAL(trace_id_), "size", size);
    auto buffer = base::MakeRefCounted<net::WrappedIOBuffer>(data);
    read_buffers_.push_back(
        base::MakeRefCounted<net::DrainableIOBuffer>(buffer.get(), size));
    auto scoped_closure_runner = base::ScopedClosureRunner(base::Bind(
        &BufferedStream::CopyRemainingTemporaryBuffer, base::Unretained(this)));
    TRACE_EVENT_NESTABLE_ASYNC_END0(kTraceEventCategory, kTraceEventName,
                                    TRACE_ID_LOCAL(trace_id_));
    return scoped_closure_runner;
  }

  void NotifyFinished() {
    static const char* kTraceEventName = "BufferedStream::NotifyFinished";
    TRACE_EVENT_NESTABLE_ASYNC_BEGIN0(kTraceEventCategory, kTraceEventName,
                                      TRACE_ID_LOCAL(trace_id_));
    finished_ = true;
    TRACE_EVENT_NESTABLE_ASYNC_END0(kTraceEventCategory, kTraceEventName,
                                    TRACE_ID_LOCAL(trace_id_));
  }

  void RunCallback() {
    static const char* kTraceEventName = "BufferedStream::RunCallback";
    TRACE_EVENT_NESTABLE_ASYNC_BEGIN0(kTraceEventCategory, kTraceEventName,
                                      TRACE_ID_LOCAL(trace_id_));
    CHECK(pending_dest_buffer_);
    if (read_buffers_.empty()) {
      CHECK(finished_);
      pending_dest_buffer_ = nullptr;
      pending_buffer_size_ = 0;
      base::ResetAndReturn(&pending_read_callback_).Run(0);
      TRACE_EVENT_NESTABLE_ASYNC_END1(kTraceEventCategory, kTraceEventName,
                                      TRACE_ID_LOCAL(trace_id_), "status",
                                      "finished");
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
    TRACE_EVENT_NESTABLE_ASYNC_END1(kTraceEventCategory, kTraceEventName,
                                    TRACE_ID_LOCAL(trace_id_), "size", size);
  }

 private:
  const void* trace_id_;
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
      on_read_callback_(base::Bind(&WebPackageResponseHandler::OnRead,
                                   base::Unretained(this))) {
  TRACE_EVENT_NESTABLE_ASYNC_BEGIN1(
      kTraceEventCategory, "WebPackageResponseHandler", TRACE_ID_LOCAL(this),
      "url", request_url_.spec());
  source_stream_ = std::make_unique<BufferedStream>(this);
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
        source_stream_ = net::GzipSourceStream::Create(
            std::move(source_stream_), source_type);
      } else {
        // TODO: Maybe we could possibly support TYPE_BROTLI etc.
        NOTREACHED();
      }
    }
  }
}

WebPackageResponseHandler::~WebPackageResponseHandler() {
  TRACE_EVENT_NESTABLE_ASYNC_END0(
      kTraceEventCategory, "WebPackageResponseHandler", TRACE_ID_LOCAL(this));
}

void WebPackageResponseHandler::RegisterCallbacks(
    ResourceCallback callback,
    CompletionCallback completion_callback) {
  static const char* kTraceEventName =
      "WebPackageResponseHandler::RegisterCallbacks";
  TRACE_EVENT_NESTABLE_ASYNC_BEGIN0(kTraceEventCategory, kTraceEventName,
                                    TRACE_ID_LOCAL(this));
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
  ReadData();
  TRACE_EVENT_NESTABLE_ASYNC_END0(kTraceEventCategory, kTraceEventName,
                                  TRACE_ID_LOCAL(this));
}

void WebPackageResponseHandler::OnDataReceived(const void* data, size_t size) {
  static const char* kTraceEventName =
      "WebPackageResponseHandler::OnDataReceived";
  TRACE_EVENT_NESTABLE_ASYNC_BEGIN1(kTraceEventCategory, kTraceEventName,
                                    TRACE_ID_LOCAL(this), "size", size);
  base::ScopedClosureRunner scoped_guard =
      buffered_stream_->OnDataReceived(static_cast<const char*>(data), size);
  if (pending_write_) {
    buffered_stream_->RunCallback();
  }
  TRACE_EVENT_NESTABLE_ASYNC_END0(kTraceEventCategory, kTraceEventName,
                                  TRACE_ID_LOCAL(this));
}

void WebPackageResponseHandler::OnNotifyFinished(int error_code) {
  static const char* kTraceEventName =
      "WebPackageResponseHandler::OnNotifyFinished";
  TRACE_EVENT_NESTABLE_ASYNC_BEGIN1(kTraceEventCategory, kTraceEventName,
                                    TRACE_ID_LOCAL(this), "error_code",
                                    error_code);
  if (error_code != net::OK) {
    // Abort
    LOG(ERROR) << "OnNotifyFinished error " << error_code << " "
               << request_url_;
    CompleteAndNotify(error_code);
    TRACE_EVENT_NESTABLE_ASYNC_END0(kTraceEventCategory, kTraceEventName,
                                    TRACE_ID_LOCAL(this));
    return;
  } else {
    buffered_stream_->NotifyFinished();
    if (pending_write_) {
      buffered_stream_->RunCallback();
    }
  }
  TRACE_EVENT_NESTABLE_ASYNC_END0(kTraceEventCategory, kTraceEventName,
                                  TRACE_ID_LOCAL(this));
}

void WebPackageResponseHandler::OnPipeWritable(MojoResult result) {
  static const char* kTraceEventName = "OnPipeWritable";
  TRACE_EVENT_NESTABLE_ASYNC_BEGIN0(kTraceEventCategory, kTraceEventName,
                                    TRACE_ID_LOCAL(this));
  ReadData();
  TRACE_EVENT_NESTABLE_ASYNC_END0(kTraceEventCategory, kTraceEventName,
                                  TRACE_ID_LOCAL(this));
}

void WebPackageResponseHandler::ReadData() {
  static const char* kTraceEventName = "WebPackageResponseHandler::ReadData";
  TRACE_EVENT_NESTABLE_ASYNC_BEGIN0(kTraceEventCategory, kTraceEventName,
                                    TRACE_ID_LOCAL(this));
  if (!connected_to_consumer()) {
    TRACE_EVENT_NESTABLE_ASYNC_END1(kTraceEventCategory, kTraceEventName,
                                    TRACE_ID_LOCAL(this), "status",
                                    "not connected");
    return;
  }
  if (pending_write_) {
    TRACE_EVENT_NESTABLE_ASYNC_END1(kTraceEventCategory, kTraceEventName,
                                    TRACE_ID_LOCAL(this), "status", "pending");
    return;
  }

  while (!finished_) {
    uint32_t mojo_buffer_size = 0;
    scoped_refptr<network::NetToMojoPendingBuffer> write_buffer;
    MojoResult result = network::NetToMojoPendingBuffer::BeginWrite(
        &producer_handle_, &write_buffer, &mojo_buffer_size);
    if (result == MOJO_RESULT_SHOULD_WAIT) {
      // The pipe is full.
      writable_handle_watcher_->ArmOrNotify();
      TRACE_EVENT_NESTABLE_ASYNC_END1(kTraceEventCategory, kTraceEventName,
                                      TRACE_ID_LOCAL(this), "status",
                                      "pipe is full");
      return;
    }
    if (result != MOJO_RESULT_OK) {
      // Abort
      LOG(ERROR) << "BeginWrite error " << result << " " << request_url_;
      CompleteAndNotify(net::ERR_FAILED);
      TRACE_EVENT_NESTABLE_ASYNC_END2(kTraceEventCategory, kTraceEventName,
                                      TRACE_ID_LOCAL(this), "status", "error",
                                      "result", result);
      return;
    }

    auto dest =
        base::MakeRefCounted<network::NetToMojoIOBuffer>(write_buffer.get());
    TRACE_EVENT_NESTABLE_ASYNC_BEGIN1(kTraceEventCategory, "SourceStream::Read",
                                      TRACE_ID_LOCAL(this), "mojo_buffer_size",
                                      mojo_buffer_size);
    int rv = source_stream_->Read(dest.get(),
                                  base::checked_cast<int>(mojo_buffer_size),
                                  on_read_callback_);
    TRACE_EVENT_NESTABLE_ASYNC_END1(kTraceEventCategory, "SourceStream::Read",
                                    TRACE_ID_LOCAL(this), "rv", rv);
    if (rv == net::ERR_IO_PENDING) {
      pending_write_ = std::move(write_buffer);
      break;
    } else if (rv < net::OK) {
      // Abort
      LOG(ERROR) << "SourceStream::Read error " << rv << " " << request_url_;
      CompleteAndNotify(rv);
      TRACE_EVENT_NESTABLE_ASYNC_END1(kTraceEventCategory, kTraceEventName,
                                      TRACE_ID_LOCAL(this), "status",
                                      "SourceStream::Read");
      return;
    } else {
      TRACE_EVENT_NESTABLE_ASYNC_BEGIN1(kTraceEventCategory,
                                        "NetToMojoPendingBuffer::Complete",
                                        TRACE_ID_LOCAL(this), "size", rv);
      producer_handle_ = write_buffer->Complete(rv);
      TRACE_EVENT_NESTABLE_ASYNC_END0(kTraceEventCategory,
                                      "NetToMojoPendingBuffer::Complete",
                                      TRACE_ID_LOCAL(this));
      finished_ = rv == net::OK;
    }
  }

  if (finished_) {
    CompleteAndNotify(net::OK);
  }
  TRACE_EVENT_NESTABLE_ASYNC_END0(kTraceEventCategory, kTraceEventName,
                                  TRACE_ID_LOCAL(this));
}

void WebPackageResponseHandler::OnRead(int rv) {
  static const char* kTraceEventName = "WebPackageResponseHandler::OnRead";
  const auto trace_event_id = TRACE_ID_LOCAL(this);
  TRACE_EVENT_NESTABLE_ASYNC_BEGIN1(kTraceEventCategory, kTraceEventName,
                                    trace_event_id, "rv", rv);
  scoped_refptr<network::NetToMojoPendingBuffer> write_buffer =
      std::move(pending_write_);
  if (rv < net::OK) {
    // Abort
    LOG(ERROR) << "OnRead error " << rv << " " << request_url_;
    CompleteAndNotify(rv);
    TRACE_EVENT_NESTABLE_ASYNC_END1(kTraceEventCategory, kTraceEventName,
                                    trace_event_id, "status", "error");
    return;
  } else {
    TRACE_EVENT_NESTABLE_ASYNC_BEGIN1(kTraceEventCategory,
                                      "NetToMojoPendingBuffer::Complete",
                                      trace_event_id, "size", rv);
    producer_handle_ = write_buffer->Complete(rv);
    TRACE_EVENT_NESTABLE_ASYNC_END0(kTraceEventCategory,
                                    "NetToMojoPendingBuffer::Complete",
                                    trace_event_id);
    finished_ = rv == net::OK;
  }
  ReadData();
  TRACE_EVENT_NESTABLE_ASYNC_END0(kTraceEventCategory, kTraceEventName,
                                  trace_event_id);
}

void WebPackageResponseHandler::CompleteAndNotify(int net_err) {
  static const char* kTraceEventName =
      "WebPackageResponseHandler::CompleteAndNotify";
  const auto trace_event_id = TRACE_ID_LOCAL(this);
  TRACE_EVENT_NESTABLE_ASYNC_BEGIN0(kTraceEventCategory, kTraceEventName,
                                    trace_event_id);
  writable_handle_watcher_.reset();
  producer_handle_.reset();
  if (completion_callback_) {
    std::move(completion_callback_)
        .Run(network::URLLoaderCompletionStatus(net_err));
  }

  base::ThreadTaskRunnerHandle::Get()->PostTask(FROM_HERE,
                                                std::move(done_callback_));
  TRACE_EVENT_NESTABLE_ASYNC_END0(kTraceEventCategory, kTraceEventName,
                                  trace_event_id);
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
