// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/webpackage_reader_adapter.h"

#include "base/bind.h"
#include "base/debug/stack_trace.h"
#include "base/location.h"
#include "base/numerics/safe_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "content/public/common/resource_request.h"
#include "net/base/io_buffer.h"
#include "net/filter/filter_source_stream.h"
#include "net/filter/gzip_source_stream.h"
#include "net/filter/source_stream.h"
#include "net/http/http_util.h"
#include "services/network/public/cpp/net_adapters.h"

namespace content {

namespace {

template <typename Map, typename Key>
base::StringPiece MapGetAsStringPiece(Map& m, const Key& key) {
  auto found = m.find(key);
  DCHECK(found != m.end());
  return base::StringPiece(found->second.c_str(), found->second.size());
}

class WebPackageSourceStream : public net::SourceStream {
 public:
  explicit WebPackageSourceStream(WebPackageResponseHandler* handler)
      : SourceStream(SourceStream::TYPE_NONE),
        handler_(handler) {}
  ~WebPackageSourceStream() override {}

  int Read(net::IOBuffer* dest_buffer,
           int buffer_size,
           const net::CompletionCallback& callback) override {
    memcpy(dest_buffer->data(), handler_->GetCurrentSourcePointer(),
           buffer_size);
    return buffer_size;
  }
  std::string Description() const override {
    return "data pipe";
  }

 private:
  // Not owned, handler owns this.
  WebPackageResponseHandler* handler_ = nullptr;
};

}  // namespace

WebPackageManifest::WebPackageManifest() = default;
WebPackageManifest::~WebPackageManifest() = default;

//----------------------------------------------------------------

WebPackageResponseHandler::WebPackageResponseHandler(
    const GURL& request_url,
    ResourceResponseHead head,
    base::OnceClosure done_callback)
    : request_url_(request_url),
      response_head_(std::move(head)),
      writable_handle_watcher_(std::make_unique<mojo::SimpleWatcher>(
          FROM_HERE,
          mojo::SimpleWatcher::ArmingPolicy::MANUAL)),
      done_callback_(std::move(done_callback)) {
  LOG(ERROR) << "[" << request_url_ << "] Created ResponseHandler.";

  source_stream_ = std::make_unique<WebPackageSourceStream>(this);
  std::string type;
  size_t iter = 0;
  while (response_head_.headers->EnumerateHeader(
          &iter, "Content-Encoding", &type)) {
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
  size_t written = 0;
  LOG(ERROR) << "[" << request_url_ << "] OnDataReceived:" << size;
  if (producer_handle_.is_valid()) {
    uint32_t available = 0;
    scoped_refptr<network::NetToMojoPendingBuffer> pending_write;
    auto result = BeginWriteToPipe(&pending_write, &available);
    if (available > 0u && !data_write_finished_) {
      DCHECK_EQ(MOJO_RESULT_OK, result);
      size_t bytes_to_write =
          std::min(base::checked_cast<size_t>(available), size);
      int written = WriteToPipe(pending_write.get(), 0, data, bytes_to_write);
      LOG(ERROR) << "- writing to the pipe: " << written;
      producer_handle_ = pending_write->Complete(written);
    }
    DCHECK(producer_handle_.is_valid());
  }

  // Drop data if the pipe is closed.
  if (data_write_finished_)
    return;

  CHECK_LE(written, size);
  size_t remaining = size - written;
  if (remaining == 0)
    return;

  auto buffer = base::MakeRefCounted<net::IOBuffer>(remaining);
  read_buffers_.push_back(
      base::MakeRefCounted<net::DrainableIOBuffer>(buffer.get(), remaining));
  memcpy(buffer->data(), static_cast<const char*>(data) + written, remaining);
  LOG(ERROR) << "- writing to the buffer: " << remaining;

  if (producer_handle_.is_valid())
    writable_handle_watcher_->ArmOrNotify();
}

void WebPackageResponseHandler::OnNotifyFinished(int error_code) {
  LOG(ERROR) << "[" << request_url_ << "] OnNotifyFinished - data finished.";
  if (error_code_ == net::OK)
    error_code_ = error_code;
  data_receive_finished_ = true;
  MaybeCompleteAndNotify();
}

const char* WebPackageResponseHandler::GetCurrentSourcePointer() {
  DCHECK(current_read_pointer_);
  return static_cast<const char*>(current_read_pointer_);
}

void WebPackageResponseHandler::OnPipeWritable(MojoResult unused) {
  LOG(ERROR) << "[" << request_url_ << "] OnPipeWritable";
  DCHECK_EQ(unused, MOJO_RESULT_OK);
  DCHECK(producer_handle_.is_valid());
  DCHECK(!data_write_finished_);
  if (read_buffers_.empty())
    return;

  uint32_t available = 0;
  scoped_refptr<network::NetToMojoPendingBuffer> pending_write;
  auto result = BeginWriteToPipe(&pending_write, &available);
  if (available == 0u || data_write_finished_)
    return;
  DCHECK_EQ(MOJO_RESULT_OK, result);
  size_t num_bytes_written = 0;
  while (!read_buffers_.empty()) {
    auto& src = read_buffers_.front();
    size_t bytes_to_write =
        std::min(base::checked_cast<size_t>(available),
                 base::checked_cast<size_t>(src->BytesRemaining()));
    int written = WriteToPipe(pending_write.get(), num_bytes_written,
                              src->data(), bytes_to_write);
    src->DidConsume(written);
    LOG(ERROR) << "- wrote " << written;
    num_bytes_written += base::checked_cast<size_t>(written);
    if (src->BytesRemaining() > 0)
      break;
    read_buffers_.pop_front();
  }

  producer_handle_ = pending_write->Complete(num_bytes_written);

  if (read_buffers_.empty() && data_receive_finished_) {
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
    MaybeCompleteAndNotify();
  }
  return result;
}

int WebPackageResponseHandler::WriteToPipe(
    network::NetToMojoPendingBuffer* dest_buffer, size_t offset,
    const void* src, size_t size) {
  DCHECK(!current_read_pointer_);
  current_read_pointer_ = src;
  auto dest = base::MakeRefCounted<network::NetToMojoIOBuffer>(
      dest_buffer, base::checked_cast<int>(offset));
  int rv = source_stream_->Read(
      dest.get(), base::checked_cast<int>(size), net::CompletionCallback());
  current_read_pointer_ = nullptr;
  DCHECK(rv > 0) << rv;
  return rv;
  // memcpy(dest, src, size);
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

WebPackageReaderAdapter::PendingCallbacks::PendingCallbacks(
    ResourceCallback callback,
    CompletionCallback completion_callback)
    : callback(std::move(callback)),
      completion_callback(std::move(completion_callback)) {}

WebPackageReaderAdapter::PendingCallbacks::~PendingCallbacks() = default;

WebPackageReaderAdapter::PendingCallbacks::PendingCallbacks(
    PendingCallbacks&& other) {
  *this = std::move(other);
}

WebPackageReaderAdapter::PendingCallbacks&
WebPackageReaderAdapter::PendingCallbacks::operator=(PendingCallbacks&& other) {
  callback = std::move(other.callback);
  completion_callback = std::move(other.completion_callback);
  return *this;
}

//--------------------------------------------------------------------

WebPackageReaderAdapter::WebPackageReaderAdapter(
    WebPackageReaderAdapterClient* client,
    mojo::ScopedDataPipeConsumerHandle handle)
    : client_(client),
      handle_(std::move(handle)),
      handle_watcher_(FROM_HERE, mojo::SimpleWatcher::ArmingPolicy::MANUAL),
      reader_(std::make_unique<wpkg::WebPackageReader>(this)),
      weak_factory_(this) {
  handle_watcher_.Watch(
      handle_.get(), MOJO_HANDLE_SIGNAL_READABLE,
      base::Bind(&WebPackageReaderAdapter::OnReadable, base::Unretained(this)));
  handle_watcher_.ArmOrNotify();
}

WebPackageReaderAdapter::~WebPackageReaderAdapter() = default;

void WebPackageReaderAdapter::GetFirstResource(
    ResourceCallback callback,
    CompletionCallback completion_callback) {
  LOG(ERROR) << "** GetFirstResource";
  if (state_ < kResponseStarted) {
    DCHECK(!pending_first_request_);
    pending_first_request_ =
        PendingCallbacks(std::move(callback), std::move(completion_callback));
    LOG(ERROR) << "- Waiting for the first request.";
    return;
  }
  DCHECK_NE(kInvalidRequestId, first_request_id_);
  auto found = response_map_.find(first_request_id_);
  DCHECK(found != response_map_.end());
  found->second->AddCallbacks(std::move(callback),
                              std::move(completion_callback));
  LOG(ERROR) << "- Added callback on the response_handler_.";
}

void WebPackageReaderAdapter::GetResource(
    const ResourceRequest& resource_request,
    ResourceCallback callback,
    CompletionCallback completion_callback) {
  LOG(ERROR) << "** GetResource: " << resource_request.url;
  RequestURLAndMethod request;
  request.first = resource_request.url;
  request.second = resource_request.method;

  auto request_found = request_map_.find(request);
  if (request_found == request_map_.end() && state_ >= kResponseStarted) {
    std::move(callback).Run(net::ERR_FILE_NOT_FOUND, ResourceResponseHead(),
                            mojo::ScopedDataPipeConsumerHandle());
    return;
  }
  if (request_found != request_map_.end()) {
    int request_id = request_found->second;
    auto response_found = response_map_.find(request_id);
    if (response_found != response_map_.end()) {
      auto& handler = response_found->second;
      handler->AddCallbacks(std::move(callback),
                            std::move(completion_callback));
      return;
    }
  }

  auto inserted = pending_request_map_.emplace(
      request,
      PendingCallbacks(std::move(callback), std::move(completion_callback)));
  DCHECK(inserted.second);
}

void WebPackageReaderAdapter::OnReadable(MojoResult unused) {
  while (true) {
    const void* buffer = nullptr;
    uint32_t available = 0;
    MojoResult result =
        handle_->BeginReadData(&buffer, &available, MOJO_READ_DATA_FLAG_NONE);
    if (result == MOJO_RESULT_SHOULD_WAIT) {
      handle_watcher_.ArmOrNotify();
      return;
    }
    if (result == MOJO_RESULT_BUSY)
      return;
    if (result == MOJO_RESULT_FAILED_PRECONDITION) {
      // The end of data.
      NotifyCompletion(net::OK);
      return;
    }
    if (result != MOJO_RESULT_OK) {
      NotifyCompletion(net::ERR_FAILED);
      return;
    }
    // TODO(kinuko): We should back-pressure and stop reading if the
    // response readers are not ready.
    if (available == 0) {
      result = handle_->EndReadData(0);
      DCHECK_EQ(result, MOJO_RESULT_OK);
      handle_watcher_.ArmOrNotify();
      return;
    }
    // LOG(ERROR) << "## Writing data to the WPKGREADER: " << available;
    reader_->ConsumeDataChunk(buffer, available);
    handle_->EndReadData(available);
    // LOG(ERROR) << "## (done)";
  }
}

void WebPackageReaderAdapter::NotifyCompletion(int error_code) {
  reader_->ConsumeEOS();
  state_ = kFinished;
  main_handle_error_code_ = error_code;
  client_->OnFinishedPackage();
}

void WebPackageReaderAdapter::OnOrigin(const std::string& origin) {
  DCHECK_EQ(kInitial, state_);
  state_ = kManifestFound;
  WebPackageManifest manifest;
  // Just for now.
  manifest.start_url = GURL(origin);
  client_->OnFoundManifest(manifest);
}

void WebPackageReaderAdapter::OnResourceRequest(
    const wpkg::HttpHeaders& request_headers,
    int request_id) {
  DCHECK(state_ == kManifestFound || state_ == kRequestStarted);
  state_ = kRequestStarted;

  RequestURLAndMethod request;
  request.second = MapGetAsStringPiece(request_headers, ":method").as_string();

  std::vector<base::StringPiece> url_strings;
  url_strings.push_back(MapGetAsStringPiece(request_headers, ":scheme"));
  url_strings.push_back("://");
  url_strings.push_back(MapGetAsStringPiece(request_headers, ":authority"));
  url_strings.push_back(MapGetAsStringPiece(request_headers, ":path"));
  request.first = GURL(base::JoinString(url_strings, ""));

  LOG(ERROR) << "** Reading request: " << request.first;

  auto inserted = request_map_.emplace(request, request_id);
  // DCHECK(inserted.second);
  if (!inserted.second) {
    // We got the same request, possibly malformed package. Just ignore this.
    return;
  }

  auto reverse_inserted = reverse_request_map_.emplace(request_id, request);
  DCHECK(reverse_inserted.second);
}

void WebPackageReaderAdapter::OnResourceResponse(
    int request_id,
    const wpkg::HttpHeaders& response_headers) {
  DCHECK(state_ == kRequestStarted || state_ == kResponseStarted);
  if (state_ == kRequestStarted) {
    DCHECK_EQ(kInvalidRequestId, first_request_id_);
    first_request_id_ = request_id;
    state_ = kResponseStarted;
    AbortNotFoundPendingRequests();
  }

  auto request_found = reverse_request_map_.find(request_id);
  // DCHECK(request_found != reverse_request_map_.end());
  if (request_found == reverse_request_map_.end()) {
    // The request might have been ignored. Just ignore.
    return;
  }
  RequestURLAndMethod request = request_found->second;
  LOG(ERROR) << "** Reading response: " << request.first;

  ResourceResponseHead response_head;
  response_head.request_start = base::TimeTicks::Now();
  response_head.response_start = base::TimeTicks::Now();
  std::string buf(base::StringPrintf(
      "HTTP/1.1 %s OK\r\n",
      MapGetAsStringPiece(response_headers, ":status").data()));
  for (const auto& item : response_headers) {
    LOG(ERROR) << item.first << ": " << item.second;
    if (item.first.find(":") == 0)
      continue;
    buf.append(item.first);
    buf.append(": ");
    buf.append(item.second);
    buf.append("\r\n");
  }
  buf.append("\r\n");
  response_head.headers = new net::HttpResponseHeaders(
      net::HttpUtil::AssembleRawHeaders(buf.c_str(), buf.size()));

  if (response_head.mime_type.empty()) {
    std::string mime_type;
    response_head.headers->GetMimeType(&mime_type);
    if (mime_type.empty())
      mime_type = "text/plain";
    response_head.mime_type = mime_type;
  }

  auto inserted = response_map_.emplace(
      request_id,
      std::make_unique<WebPackageResponseHandler>(
          request.first, std::move(response_head),
          base::BindOnce(&WebPackageReaderAdapter::OnResponseHandlerFinished,
                         base::Unretained(this), request_id)));
  DCHECK(inserted.second);
  auto& handler = inserted.first->second;

  auto pending_found = pending_request_map_.find(request);
  if (pending_found != pending_request_map_.end()) {
    LOG(ERROR) << "** pending request -- found: " << request.first;
    handler->AddCallbacks(std::move(pending_found->second.callback),
                          std::move(pending_found->second.completion_callback));
    pending_request_map_.erase(pending_found);
    return;
  }

  if (pending_first_request_ && request_id == first_request_id_) {
    LOG(ERROR) << "** first request -- found: " << request.first;
    handler->AddCallbacks(
        std::move(pending_first_request_->callback),
        std::move(pending_first_request_->completion_callback));
    pending_first_request_.reset();
    return;
  }
}

void WebPackageReaderAdapter::OnDataReceived(int request_id,
                                             const void* data,
                                             size_t size) {
  auto found = response_map_.find(request_id);
  // The response consumer may bail out.
  if (found == response_map_.end())
    return;
  found->second->OnDataReceived(data, size);
}

void WebPackageReaderAdapter::OnNotifyFinished(int request_id) {
  auto found = response_map_.find(request_id);
  // The response consumer may have bailed out.
  if (found == response_map_.end())
    return;
  found->second->OnNotifyFinished(net::OK);
}

void WebPackageReaderAdapter::OnResponseHandlerFinished(int request_id) {
  response_map_.erase(request_id);
}

void WebPackageReaderAdapter::AbortNotFoundPendingRequests() {
  auto it = pending_request_map_.begin();
  while (it != pending_request_map_.end()) {
    auto request = it->first;
    if (request_map_.find(request) == request_map_.end()) {
      std::move(it->second.callback)
          .Run(net::ERR_FILE_NOT_FOUND, ResourceResponseHead(),
               mojo::ScopedDataPipeConsumerHandle());
      pending_request_map_.erase(it++);
      continue;
    }
    it++;
  }
}

void WebPackageReaderAdapter::AbortAllPendingRequests(int error_code) {
  for (auto& r : pending_request_map_) {
    std::move(r.second.callback)
        .Run(error_code, ResourceResponseHead(),
             mojo::ScopedDataPipeConsumerHandle());
  }
  pending_request_map_.clear();
}

}  // namespace content
