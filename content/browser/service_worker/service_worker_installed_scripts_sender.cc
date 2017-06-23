// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/service_worker/service_worker_installed_scripts_sender.h"

#include "content/browser/service_worker/service_worker_context_core.h"
#include "content/browser/service_worker/service_worker_disk_cache.h"
#include "content/browser/service_worker/service_worker_script_cache_map.h"
#include "content/browser/service_worker/service_worker_storage.h"
#include "net/base/io_buffer.h"

namespace content {

namespace {

class MetaDataSender {
 public:
  MetaDataSender(scoped_refptr<net::IOBufferWithSize> meta_data,
                 mojo::ScopedDataPipeProducerHandle handle)
      : meta_data_(std::move(meta_data)),
        remaining_(meta_data_->size()),
        handle_(std::move(handle)),
        watcher_(FROM_HERE, mojo::SimpleWatcher::ArmingPolicy::AUTOMATIC),
        weak_factory_(this) {}

  void Start(base::Closure callback) {
    callback_ = std::move(callback);
    watcher_.Watch(
        handle_.get(), MOJO_HANDLE_SIGNAL_WRITABLE,
        base::Bind(&MetaDataSender::OnWritable, weak_factory_.GetWeakPtr()));
  }

  void OnWritable(MojoResult result) {
    uint32_t size = remaining_;
    MojoResult rv = mojo::WriteDataRaw(handle_.get(), meta_data_->data(), &size,
                                       MOJO_WRITE_DATA_FLAG_NONE);
    remaining_ -= size;
    if (remaining_ == 0) {
      watcher_.Cancel();
      handle_.reset();
      callback_.Run();
      return;
    }

    switch (rv) {
      case MOJO_RESULT_SHOULD_WAIT:
        return;
    }
  }

 private:
  base::Closure callback_;

  scoped_refptr<net::IOBufferWithSize> meta_data_;
  size_t remaining_;
  mojo::ScopedDataPipeProducerHandle handle_;
  mojo::SimpleWatcher watcher_;

  base::WeakPtrFactory<MetaDataSender> weak_factory_;
};

class WriterIOBuffer final : public net::IOBufferWithSize {
 public:
  WriterIOBuffer(void* data, size_t size)
      : net::IOBufferWithSize(static_cast<char*>(data), size) {}

 private:
  ~WriterIOBuffer() override { data_ = nullptr; }
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
    LOG(ERROR) << __PRETTY_FUNCTION__;
    scoped_refptr<HttpResponseInfoIOBuffer> info_buf =
        base::MakeRefCounted<HttpResponseInfoIOBuffer>();
    reader_->ReadInfo(info_buf.get(), base::Bind(&Sender::OnReadInfoComplete,
                                                 AsWeakPtr(), info_buf));
  }

  // A callback for |reader_|.
  void OnReadInfoComplete(scoped_refptr<HttpResponseInfoIOBuffer> http_info,
                          int result) {
    DCHECK(owner_);
    DCHECK(http_info);
    LOG(ERROR) << __PRETTY_FUNCTION__ << " result: " << result;
    if (!http_info->http_info) {
      LOG(ERROR) << __PRETTY_FUNCTION__ << " no http_info";

      // This will remove |this|.
      owner_->OnAbortScriptTransfer();
      return;
    }

    mojo::DataPipe meta_data_pipe;
    mojo::DataPipe body_pipe;
    body_handle_ = std::move(body_pipe.producer_handle);

    // Start sending meta data
    if (http_info->http_info->metadata) {
      LOG(ERROR) << __PRETTY_FUNCTION__
                 << "metadata->size: " << http_info->http_info->metadata->size()
                 << " ";
      meta_data_sender_ = base::MakeUnique<MetaDataSender>(
          http_info->http_info->metadata,
          std::move(meta_data_pipe.producer_handle));
      meta_data_sender_->Start(
          base::Bind(&Sender::OnMetaDataTransferred, AsWeakPtr()));
    }

    // Start sending body
    watcher_.Watch(body_handle_.get(), MOJO_HANDLE_SIGNAL_WRITABLE,
                   base::Bind(&Sender::OnWritableBody, AsWeakPtr()));
    watcher_.ArmOrNotify();

    owner_->OnStartScriptTransfer("", std::move(meta_data_pipe.consumer_handle),
                                  std::move(body_pipe.consumer_handle));
    owner_->OnHttpInfoRead(http_info);
  }

  void OnWritableBody(MojoResult /* unused */) {
    LOG(ERROR) << __PRETTY_FUNCTION__;
    void* buf = nullptr;
    uint32_t available = 0;
    MojoResult rv = mojo::BeginWriteDataRaw(
        body_handle_.get(), &buf, &available, MOJO_WRITE_DATA_FLAG_NONE);
    switch (rv) {
      case MOJO_RESULT_SHOULD_WAIT:
        watcher_.ArmOrNotify();
        break;
    }
    LOG(ERROR) << __PRETTY_FUNCTION__ << "available: " << available;
    buffer_ = base::MakeRefCounted<WriterIOBuffer>(buf, available);
    reader_->ReadData(buffer_.get(), available,
                      base::Bind(&Sender::OnReadComplete, AsWeakPtr()));
  }

  // A callback for |reader_|.
  void OnReadComplete(int result) {
    LOG(ERROR) << __PRETTY_FUNCTION__ << " result: " << result;
    if (result < 0) {
      LOG(ERROR) << "error: ReadData";
      return;
    }

    MojoResult rv = mojo::EndWriteDataRaw(body_handle_.get(), result);
    if (rv != MOJO_RESULT_OK) {
      LOG(ERROR) << "error: EndWriteDataRaw";
      return;
    }

    buffer_ = nullptr;
    if (result == 0) {
      watcher_.Cancel();
      body_handle_.reset();
      if (!meta_data_sender_)
        CompleteTransfer();
      return;
    }
    watcher_.ArmOrNotify();
  }

  void CompleteTransfer() {
    LOG(ERROR) << __PRETTY_FUNCTION__;
    owner_->OnFinishScriptTransfer();
  }

  void OnMetaDataTransferred() {
    meta_data_sender_ = nullptr;
    if (!body_handle_.is_valid())
      CompleteTransfer();
  }

 private:
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
      cache_map_(std::move(cache_map)),
      context_(std::move(context)) {}

ServiceWorkerInstalledScriptsSender::~ServiceWorkerInstalledScriptsSender() {}

mojom::ServiceWorkerInstalledScriptsInfoPtr
ServiceWorkerInstalledScriptsSender::CreateInfoAndBind() {
  DCHECK(cache_map_);
  auto info = mojom::ServiceWorkerInstalledScriptsInfo::New();
  info->manager_request = mojo::MakeRequest(&manager_);

  // Read all installed urls.
  int64_t main_script_id = kInvalidServiceWorkerResourceId;
  std::vector<ServiceWorkerDatabase::ResourceRecord> resources;
  cache_map_->GetResources(&resources);
  for (const auto& resource : resources) {
    info->installed_urls.emplace_back(resource.url);
    if (resource.url == main_script_url_) {
      main_script_id = resource.resource_id;
    } else {
      imported_urls_.emplace(resource.resource_id, resource.url);
    }
  }

  // No script has been installed.
  if (main_script_id == kInvalidServiceWorkerResourceId)
    return info;

  // Sort the ids for pushing ascending order which is the same with when it's
  // installed.
  // std::sort(imported_urls_.begin(), imported_urls_.end());

  // Start pushing the data from the main script.
  current_imported_url_ = imported_urls_.end();
  StartPushScript(main_script_id);
  return info;
}

void ServiceWorkerInstalledScriptsSender::StartPushScript(int64_t resource_id) {
  DCHECK(!running_sender_);
  auto reader = context_->storage()->CreateResponseReader(resource_id);
  running_sender_ = base::MakeUnique<Sender>(std::move(reader), this);
  running_sender_->Start();
}

void ServiceWorkerInstalledScriptsSender::OnStartScriptTransfer(
    std::string encoding,
    mojo::ScopedDataPipeConsumerHandle meta_data_handle,
    mojo::ScopedDataPipeConsumerHandle body_handle) {
  const GURL& script_url =
      is_main_script() ? main_script_url_ : current_imported_url_->second;

  manager_->TransferInstalledScript(script_url, encoding,
                                    std::move(body_handle),
                                    std::move(meta_data_handle));
}

void ServiceWorkerInstalledScriptsSender::OnHttpInfoRead(
    scoped_refptr<HttpResponseInfoIOBuffer> http_info) {
  if (is_main_script())
    owner_->SetMainScriptHttpResponseInfo(*http_info->http_info);
}

void ServiceWorkerInstalledScriptsSender::OnFinishScriptTransfer() {
  running_sender_.reset();
  if (is_main_script()) {
    // Imported scripts should be served after the main script.
    current_imported_url_ = imported_urls_.begin();
  } else {
    current_imported_url_++;
  }
  if (current_imported_url_ == imported_urls_.end())
    return;
  StartPushScript(current_imported_url_->first);
}

void ServiceWorkerInstalledScriptsSender::OnAbortScriptTransfer() {
  OnFinishScriptTransfer();
}

bool ServiceWorkerInstalledScriptsSender::is_main_script() {
  return current_imported_url_ == imported_urls_.end();
}

}  // namespace content
