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

}  // namespace

class ServiceWorkerInstalledScriptsSender::Sender {
 public:
  Sender(std::unique_ptr<ServiceWorkerResponseReader> reader,
         ServiceWorkerInstalledScriptsSender* owner)
      : reader_(std::move(reader)), owner_(owner), weak_factory_(this) {}
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
    // net::IOBuffer *buf, int buf_len, const net::CompletionCallback &callback
    // reader_->ReadData();
    owner_->OnStartScriptTransfer("", std::move(meta_data_pipe.consumer_handle),
                                  std::move(body_pipe.consumer_handle));
  }

  void OnWritable(MojoResult result) { LOG(ERROR) << __PRETTY_FUNCTION__; }
  // A callback for |reader_|.
  void OnReadComplete(int result) { LOG(ERROR) << __PRETTY_FUNCTION__; }

  void CompleteTransfer() {
    LOG(ERROR) << __PRETTY_FUNCTION__;
    owner_->OnFinishScriptTransfer();
  }

  void OnMetaDataTransferred() {}

 private:
  base::WeakPtr<Sender> AsWeakPtr() { return weak_factory_.GetWeakPtr(); }

  std::unique_ptr<ServiceWorkerResponseReader> reader_;
  ServiceWorkerInstalledScriptsSender* owner_;

  std::unique_ptr<MetaDataSender> meta_data_sender_;

  mojo::ScopedDataPipeProducerHandle meta_data_handle_;
  mojo::ScopedDataPipeProducerHandle body_handle_;

  base::WeakPtrFactory<Sender> weak_factory_;
};

ServiceWorkerInstalledScriptsSender::ServiceWorkerInstalledScriptsSender(
    const GURL& main_script_url,
    base::WeakPtr<ServiceWorkerScriptCacheMap> cache_map,
    base::WeakPtr<ServiceWorkerContextCore> context)
    : main_script_url_(main_script_url),
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
  manager_->TransferInstalledScript(current_imported_url_->second, encoding,
                                    std::move(body_handle),
                                    std::move(meta_data_handle));
}
void ServiceWorkerInstalledScriptsSender::OnFinishScriptTransfer() {}
void ServiceWorkerInstalledScriptsSender::OnAbortScriptTransfer() {
  running_sender_.reset();
  if (current_imported_url_ == imported_urls_.end()) {
    // Imported scripts should be served after the main script.
    current_imported_url_ = imported_urls_.begin();
  } else {
    current_imported_url_++;
  }
  if (current_imported_url_ == imported_urls_.end())
    return;
  StartPushScript(current_imported_url_->first);
}

}  // namespace content
