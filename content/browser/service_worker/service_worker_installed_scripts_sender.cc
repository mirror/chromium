// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/service_worker/service_worker_installed_scripts_sender.h"

#include "base/trace_event/trace_event.h"
#include "content/browser/service_worker/service_worker_context_core.h"
#include "content/browser/service_worker/service_worker_disk_cache.h"
#include "content/browser/service_worker/service_worker_script_cache_map.h"
#include "content/browser/service_worker/service_worker_storage.h"
#include "net/base/io_buffer.h"
#include "net/http/http_response_headers.h"

namespace content {

namespace {

class MetaDataSender {
 public:
  enum class Status { kSuccess, kFailed };

  MetaDataSender(scoped_refptr<net::IOBufferWithSize> meta_data,
                 mojo::ScopedDataPipeProducerHandle handle)
      : meta_data_(std::move(meta_data)),
        remaining_(meta_data_->size()),
        handle_(std::move(handle)),
        watcher_(FROM_HERE, mojo::SimpleWatcher::ArmingPolicy::AUTOMATIC),
        weak_factory_(this) {}

  void Start(base::OnceCallback<void(Status)> callback) {
    callback_ = std::move(callback);
    watcher_.Watch(
        handle_.get(), MOJO_HANDLE_SIGNAL_WRITABLE,
        base::Bind(&MetaDataSender::OnWritable, weak_factory_.GetWeakPtr()));
  }

  void OnWritable(MojoResult result) {
    DCHECK_EQ(MOJO_RESULT_OK, result);
    uint32_t size = remaining_;
    MojoResult rv = mojo::WriteDataRaw(handle_.get(), meta_data_->data(), &size,
                                       MOJO_WRITE_DATA_FLAG_NONE);
    switch (rv) {
      case MOJO_RESULT_INVALID_ARGUMENT:
      case MOJO_RESULT_OUT_OF_RANGE:
      case MOJO_RESULT_BUSY:
        NOTREACHED();
        return;
      case MOJO_RESULT_FAILED_PRECONDITION:
        OnCompleted(Status::kFailed);
        return;
      case MOJO_RESULT_SHOULD_WAIT:
        return;
    }
    remaining_ -= size;
    if (remaining_ == 0) {
      OnCompleted(Status::kSuccess);
      return;
    }
  }

  void OnCompleted(Status status) {
    watcher_.Cancel();
    handle_.reset();
    std::move(callback_).Run(status);
  }

 private:
  base::OnceCallback<void(Status)> callback_;

  scoped_refptr<net::IOBufferWithSize> meta_data_;
  size_t remaining_;
  mojo::ScopedDataPipeProducerHandle handle_;
  mojo::SimpleWatcher watcher_;

  base::WeakPtrFactory<MetaDataSender> weak_factory_;
};

// This class wraps raw buffer by net::IOBufferWithSize to read body from
// ServiceWorkerResponseReader and write the data to the raw buffer. The
// destructor of net::IOBufferWithSize usually deletes its internal buffer, but
// this class avoids that because the buffer is borrowed from
// mojo::BeginWriteDataRaw.
class WriterIOBuffer : public net::IOBufferWithSize {
 public:
  WriterIOBuffer(void* buffer, uint32_t size)
      : net::IOBufferWithSize(static_cast<char*>(buffer),
                              static_cast<size_t>(size)) {}

 private:
  ~WriterIOBuffer() override {
    // |data_| should not be deleted by the destructor.
    data_ = nullptr;
  }
  DISALLOW_COPY_AND_ASSIGN(WriterIOBuffer);
};

}  // namespace

class ServiceWorkerInstalledScriptsSender::Sender {
 public:
  Sender(std::unique_ptr<ServiceWorkerResponseReader> reader,
         ServiceWorkerInstalledScriptsSender* owner)
      : reader_(std::move(reader)),
        owner_(owner),
        watcher_(FROM_HERE, mojo::SimpleWatcher::ArmingPolicy::MANUAL),
        weak_factory_(this) {}

  void Start() {
    scoped_refptr<HttpResponseInfoIOBuffer> info_buf =
        base::MakeRefCounted<HttpResponseInfoIOBuffer>();
    reader_->ReadInfo(info_buf.get(), base::Bind(&Sender::OnReadInfoComplete,
                                                 AsWeakPtr(), info_buf));
  }

 private:
  enum class Status { kSuccess, kFailed };

  void OnReadInfoComplete(scoped_refptr<HttpResponseInfoIOBuffer> http_info,
                          int result) {
    DCHECK(owner_);
    DCHECK(http_info);
    DCHECK_GT(result, 0);
    if (!http_info->http_info) {
      CompleteTransfer(Status::kFailed);
      return;
    }

    mojo::ScopedDataPipeConsumerHandle meta_data_consumer;
    mojo::ScopedDataPipeConsumerHandle body_consumer;
    if (mojo::CreateDataPipe(nullptr, &body_handle_, &body_consumer) !=
        MOJO_RESULT_OK) {
      CompleteTransfer(Status::kFailed);
      return;
    }
    // Start sending meta data which is V8 code cache.
    if (http_info->http_info->metadata) {
      mojo::ScopedDataPipeProducerHandle meta_data_producer;
      if (mojo::CreateDataPipe(nullptr, &meta_data_producer,
                               &meta_data_consumer) != MOJO_RESULT_OK) {
        CompleteTransfer(Status::kFailed);
        return;
      }
      meta_data_sender_ = base::MakeUnique<MetaDataSender>(
          http_info->http_info->metadata, std::move(meta_data_producer));
      meta_data_sender_->Start(
          base::BindOnce(&Sender::OnMetaDataTransferred, AsWeakPtr()));
    }

    // Start sending body.
    watcher_.Watch(body_handle_.get(), MOJO_HANDLE_SIGNAL_WRITABLE,
                   base::Bind(&Sender::OnWritableBody, AsWeakPtr()));
    watcher_.ArmOrNotify();

    scoped_refptr<net::HttpResponseHeaders> headers =
        http_info->http_info->headers;
    DCHECK(headers);

    std::string charset;
    if (!headers->GetCharset(&charset))
      charset = "";

    // Create a map of response headers
    std::unordered_map<std::string, std::string> header_strings;
    size_t iter = 0;
    std::string key;
    std::string value;
    // This logic is copied from blink::ResourceResponse::AddHTTPHeaderField.
    while (headers->EnumerateHeaderLines(&iter, &key, &value)) {
      if (header_strings.find(key) == header_strings.end()) {
        header_strings[key] = value;
      } else {
        header_strings[key] += ", " + value;
      }
    }

    owner_->StartScriptTransfer(charset, std::move(header_strings),
                                std::move(meta_data_consumer),
                                std::move(body_consumer));
    owner_->OnHttpInfoRead(http_info);
  }

  void OnWritableBody(MojoResult result) {
    DCHECK_EQ(MOJO_RESULT_OK, result);
    void* buf = nullptr;
    uint32_t num_bytes = 0;
    MojoResult rv = mojo::BeginWriteDataRaw(
        body_handle_.get(), &buf, &num_bytes, MOJO_WRITE_DATA_FLAG_NONE);
    switch (rv) {
      case MOJO_RESULT_INVALID_ARGUMENT:
      case MOJO_RESULT_BUSY:
        NOTREACHED();
        return;
      case MOJO_RESULT_FAILED_PRECONDITION:
        CompleteTransfer(Status::kFailed);
        return;
      case MOJO_RESULT_SHOULD_WAIT:
        watcher_.ArmOrNotify();
        return;
      case MOJO_RESULT_OK:
        break;
    }
    buffer_ = base::MakeRefCounted<WriterIOBuffer>(buf, num_bytes);
    reader_->ReadData(buffer_.get(), num_bytes,
                      base::Bind(&Sender::OnResponseDataRead, AsWeakPtr()));
  }

  void OnResponseDataRead(int written_bytes) {
    if (written_bytes < 0) {
      CompleteTransfer(Status::kFailed);
      return;
    }
    MojoResult rv = mojo::EndWriteDataRaw(body_handle_.get(), written_bytes);
    DCHECK_EQ(MOJO_RESULT_OK, rv);
    buffer_ = nullptr;
    if (written_bytes == 0) {
      // All data has been read.
      watcher_.Cancel();
      body_handle_.reset();
      if (!meta_data_sender_)
        CompleteTransfer(Status::kSuccess);
      return;
    }
    watcher_.ArmOrNotify();
  }

  void OnMetaDataTransferred(MetaDataSender::Status status) {
    meta_data_sender_.reset();
    if (status == MetaDataSender::Status::kFailed) {
      watcher_.Cancel();
      body_handle_.reset();
      CompleteTransfer(Status::kFailed);
      return;
    }

    if (!body_handle_.is_valid())
      CompleteTransfer(Status::kSuccess);
  }

  // CompleteTransfer notifies the end of data transfer to |owner_|, and |this|
  // will be removed by |owner_| as a result.
  void CompleteTransfer(Status status) {
    switch (status) {
      case Status::kSuccess:
        owner_->OnFinishScriptTransfer();
        return;
      case Status::kFailed:
        owner_->OnAbortScriptTransfer();
        return;
    }
    NOTREACHED();
  }

  base::WeakPtr<Sender> AsWeakPtr() { return weak_factory_.GetWeakPtr(); }

  std::unique_ptr<ServiceWorkerResponseReader> reader_;
  ServiceWorkerInstalledScriptsSender* owner_;

  // For meta data
  std::unique_ptr<MetaDataSender> meta_data_sender_;

  // For body
  scoped_refptr<net::IOBufferWithSize> buffer_;
  mojo::SimpleWatcher watcher_;

  // Pipes
  mojo::ScopedDataPipeProducerHandle meta_data_handle_;
  mojo::ScopedDataPipeProducerHandle body_handle_;

  base::WeakPtrFactory<Sender> weak_factory_;
};

ServiceWorkerInstalledScriptsSender::ServiceWorkerInstalledScriptsSender(
    ServiceWorkerVersion* owner,
    const GURL& main_script_url,
    base::WeakPtr<ServiceWorkerScriptCacheMap> cache_map,
    base::WeakPtr<ServiceWorkerContextCore> context)
    : owner_(owner),
      main_script_url_(main_script_url),
      main_script_id_(kInvalidServiceWorkerResourceId),
      cache_map_(std::move(cache_map)),
      context_(std::move(context)) {}

ServiceWorkerInstalledScriptsSender::~ServiceWorkerInstalledScriptsSender() {}

mojom::ServiceWorkerInstalledScriptsInfoPtr
ServiceWorkerInstalledScriptsSender::CreateInfoAndBind() {
  DCHECK(cache_map_);
  TRACE_EVENT_NESTABLE_ASYNC_BEGIN1("ServiceWorker",
                                    "ServiceWorkerInstalledScriptsSender", this,
                                    "main_script_url", main_script_url_.spec());

  std::vector<ServiceWorkerDatabase::ResourceRecord> resources;
  std::vector<GURL> installed_urls;
  cache_map_->GetResources(&resources);
  for (const auto& resource : resources) {
    installed_urls.emplace_back(resource.url);
    if (resource.url == main_script_url_) {
      main_script_id_ = resource.resource_id;
    } else {
      imported_urls_.emplace(resource.resource_id, resource.url);
    }
  }

  if (installed_urls.size() == 0)
    return nullptr;

  auto info = mojom::ServiceWorkerInstalledScriptsInfo::New();
  info->manager_request = mojo::MakeRequest(&manager_);
  info->installed_urls = std::move(installed_urls);
  return info;
}

void ServiceWorkerInstalledScriptsSender::Start() {
  // Return if no script has been installed.
  if (main_script_id_ == kInvalidServiceWorkerResourceId)
    return;
  current_streamed_url_ = imported_urls_.end();
  StartPushScript(main_script_id_);
}

void ServiceWorkerInstalledScriptsSender::StartPushScript(int64_t resource_id) {
  DCHECK(!running_sender_);
  auto reader = context_->storage()->CreateResponseReader(resource_id);
  const GURL& script_url =
      is_main_script() ? main_script_url_ : current_streamed_url_->second;
  TRACE_EVENT_NESTABLE_ASYNC_BEGIN1("ServiceWorker", "ScriptTransfer", this,
                                    "script_url", script_url.spec());
  running_sender_ = base::MakeUnique<Sender>(std::move(reader), this);
  running_sender_->Start();
}

void ServiceWorkerInstalledScriptsSender::StartScriptTransfer(
    std::string encoding,
    std::unordered_map<std::string, std::string> headers,
    mojo::ScopedDataPipeConsumerHandle meta_data_handle,
    mojo::ScopedDataPipeConsumerHandle body_handle) {
  TRACE_EVENT_NESTABLE_ASYNC_INSTANT0("ServiceWorker",
                                      "CallTransferInstalledScript", this);
  const GURL& script_url =
      is_main_script() ? main_script_url_ : current_streamed_url_->second;
  auto script_info = mojom::ServiceWorkerScriptInfo::New();
  script_info->script_url = script_url;
  script_info->headers = std::move(headers);
  script_info->encoding = std::move(encoding);
  script_info->body = std::move(body_handle);
  script_info->meta_data = std::move(meta_data_handle);
  manager_->TransferInstalledScript(std::move(script_info));
}

void ServiceWorkerInstalledScriptsSender::OnHttpInfoRead(
    scoped_refptr<HttpResponseInfoIOBuffer> http_info) {
  if (is_main_script())
    owner_->SetMainScriptHttpResponseInfo(*http_info->http_info);
}

void ServiceWorkerInstalledScriptsSender::OnFinishScriptTransfer() {
  TRACE_EVENT_NESTABLE_ASYNC_END0("ServiceWorker", "ScriptTransfer", this);
  running_sender_.reset();
  if (is_main_script()) {
    // Imported scripts will be served after the main script.
    current_streamed_url_ = imported_urls_.begin();
  } else {
    current_streamed_url_++;
  }
  if (current_streamed_url_ == imported_urls_.end()) {
    TRACE_EVENT_NESTABLE_ASYNC_END0(
        "ServiceWorker", "ServiceWorkerInstalledScriptsSender", this);
    return;
  }
  // Start streaming the next script.
  StartPushScript(current_streamed_url_->first);
}

void ServiceWorkerInstalledScriptsSender::OnAbortScriptTransfer() {
  // TODO(shimazu): Report the error to ServiceWorkerVersion.
  OnFinishScriptTransfer();
}

bool ServiceWorkerInstalledScriptsSender::is_main_script() {
  return current_streamed_url_ == imported_urls_.end();
}

}  // namespace content
