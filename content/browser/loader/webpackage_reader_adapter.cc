// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/webpackage_reader_adapter.h"

#include "base/bind.h"
#include "base/location.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "content/public/common/resource_request.h"
#include "net/base/io_buffer.h"
#include "services/network/public/cpp/net_adapters.h"

namespace content {

namespace {

template <typename Map, typename Key>
base::StringPiece MapGetAsStringPiece(Map& m, const KEY& key) {
  auto found = m.find(key);
  DCHECK(found != m.end());
  return base::StringPiece(found->second.c_str(), found->second.size());
}

}  // namespace

WebPackageManifest::WebPackageManifest() = default;
WebPackageManifest::~WebPackageManifest() = default;

WebPackageReaderAdapter::PendingResource::PendingResource(
    ResourceResponseHead head)
    : response_head_(std::move(head)),
      writable_handle_watcher_(FROM_HERE,
                               mojo::SimpleWatcher::ArmingPolicy::MANUAL) {}

WebPackageReaderAdapter::PendingResource::~PendingResource() = default;

WebPackageReaderAdapter::PendingResource::PendingResource(
    PendingResource&& other) {
  *this = std::move(other);
}

PendingResource& WebPackageReaderAdapter::PendingResource::operator=(
    PendingResource&& other) {
  response_head_ = std::move(other.response_head_);
  completion_status_ = std::move(other.completion_status_);
  producer_handle_ = std::move(other.producer_handle_);
  consumer_handle_ = std::move(other.consumer_handle_);
  writable_handle_watcher_ = std::move(other.writable_handle_watcher_);
  pending_write_ = std::move(pending_write_);
  pending_callbacks_ = std::move(other.pending_callbacks_);
  pending_completion_callbacks_ =
      std::move(other.pending_completion_callbacks_);
  return *this;
}

mojo::ScopedDataPipeConsumerHandle
WebPackageReaderAdapter::PendingResource::StartResponseBody() {
  DCHECK(!consumer_handle_);
  if (!producer_handle_) {
    mojo::DataPipe data_pipe(kDefaultAllocationSize);
    producer_handle_ = std::move(data_pipe.producer_handle);
    consumer_handle_ = std::move(data_pipe.consumer_handle);
    writable_handle_watcher_.Watch(
        producer_handle_.get(), MOJO_HANDLE_SIGNAL_WRITABLE,
        base::Bind(&PendingResource::OnResponseBodyStreamReady,
                   base::Unretained(this)));
  }
  return std::move(consumer_handle_);
}

void WebPackageReaderAdapter::PendingResource::OnDataReceived(const void* data,
                                                              size_t size) {
  read_buffers_.push_back(base::MakeRefCounted<net::IOBufferWithSize>(size));
  memcpy(read_buffers_.back()->data());

  StreamResponseBody();
}

void WebPackageReaderAdapter::PendingResource::OnNotifyFinished(
    int error_code) {}

void WebPackageReaderAdapter::OnResponseBodyStreamReady(MojoResult unused) {
  DCHECK_EQ(unused, MOJO_RESULT_OK);
}

void WebPackageReaderAdapter::StreamResponseBody() {}

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
  if (state_ < kResponseStarted) {
    DCHECK(!waiting_first_response_);
    waiting_first_response_ = base::BindOnce(
        &WebPackageReaderAdapter::OnFirstResponse, weak_factory_.GetWeakPtr(),
        std::move(callback), std::move(completion_callback));
    return;
  }
  OnFirstResponse(std::move(callback), std::move(completion_callback));
}

void WebPackageReaderAdapter::GetResource(
    const ResourceRequest& resource_request,
    ResourceCallback callback,
    CompletionCallback completion_callback) {
  // TODO(kinuko): implement.
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
    reader_->ConsumeDataChunk(buffer, available);
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

  std::pair<GURL, std::string> request;
  request.second = request_headers[":method"];

  std::vector<base::StringPiece> url_strings;
  url_strings.push_back(MapGetAsStringPiece(request_headers, ":scheme"));
  url_strings.push_back("://");
  url_strings.push_back(MapGetAsStringPiece(request_headers, ":authority"));
  url_strings.push_back(MapGetAsStringPiece(request_headers, ":path"));
  request.first = GURL(base::JoinString(url_strings));

  auto inserted = request_map_.emplace(request, request_id);
  DCHECK(inserted.second);
}

void WebPackageReaderAdapter::OnResourceResponse(
    int request_id,
    const wpkg::HttpHeaders& response_headers) {
  DCHECK(state_ == kManifestFound || state_ == kRequestStarted);
  if (state_ == kManifestFound) {
    DCHECK_EQ(kInvalidRequestId, first_request_id_);
    first_request_id_ = request_id;
    state_ = kResponseStarted;
  }

  ResourceResponseHead response_head;
  response_head.request_start = base::TimeTicks::Now();
  response_head.response_start = base::TimeTicks::Now();
  std::string buf(base::StringPrintf("HTTP/1.1 %s OK\r\n"),
                  response_headers[":status"]);
  for (const auto& item : headers) {
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

  auto inserted = response_map_.emplace(
      request_id, PendingResource(std::move(response_head)));
  DCHECK(inserted.second);

  if (waiting_first_response_)
    std::move(waiting_first_response_).Run();
}

void WebPackageReaderAdapter::OnFirstResponse(
    ResourceCallback callback,
    CompletionCallback completion_callback) {}

}  // namespace content
