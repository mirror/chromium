// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/service_worker/service_worker_script_url_loader.h"

#include <memory>
#include "base/barrier_closure.h"
#include "content/browser/appcache/appcache_response.h"
#include "content/browser/service_worker/service_worker_cache_writer.h"
#include "content/browser/service_worker/service_worker_context_core.h"
#include "content/browser/service_worker/service_worker_disk_cache.h"
#include "content/browser/service_worker/service_worker_storage.h"
#include "content/browser/service_worker/service_worker_version.h"
#include "content/browser/service_worker/service_worker_write_to_cache_job.h"
#include "content/browser/url_loader_factory_getter.h"
#include "content/public/common/resource_response.h"

namespace content {

ServiceWorkerScriptURLLoader::ServiceWorkerScriptURLLoader(
    int32_t routing_id,
    int32_t request_id,
    uint32_t options,
    const ResourceRequest& resource_request,
    mojom::URLLoaderClientPtr client,
    scoped_refptr<ServiceWorkerVersion> version,
    scoped_refptr<URLLoaderFactoryGetter> loader_factory_getter,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation)
    : request_url_(resource_request.url),
      resource_type_(resource_request.resource_type),
      resource_id_(version->context()->storage()->NewResourceId()),
      version_(version),
      network_client_binding_(this),
      watcher_(FROM_HERE, mojo::SimpleWatcher::ArmingPolicy::MANUAL),
      forwarding_client_(std::move(client)),
      completion_closure_(base::BarrierClosure(
          2,
          base::BindOnce(
              &ServiceWorkerScriptURLLoader::OnWriteHeadersAndDataComplete,
              base::Unretained(this)))),
      weak_factory_(this) {
  DCHECK_EQ(ServiceWorkerVersion::NEW, version->status());
  DCHECK(resource_type_ == RESOURCE_TYPE_SERVICE_WORKER ||
         resource_type_ == RESOURCE_TYPE_SCRIPT);
  // TODO(nhiroki): Handle the case where |resource_id_| is invalid.
  InitializeScriptCacheWriter();
  version_->script_cache_map()->NotifyStartedCaching(request_url_,
                                                     resource_id_);

  mojom::URLLoaderClientPtr network_client;
  network_client_binding_.Bind(mojo::MakeRequest(&network_client));
  loader_factory_getter->GetNetworkFactory()->get()->CreateLoaderAndStart(
      mojo::MakeRequest(&network_loader_), routing_id, request_id, options,
      resource_request, std::move(network_client), traffic_annotation);
}

ServiceWorkerScriptURLLoader::~ServiceWorkerScriptURLLoader() = default;

void ServiceWorkerScriptURLLoader::FollowRedirect() {
  network_loader_->FollowRedirect();
}

void ServiceWorkerScriptURLLoader::SetPriority(net::RequestPriority priority,
                                               int32_t intra_priority_value) {
  network_loader_->SetPriority(priority, intra_priority_value);
}

void ServiceWorkerScriptURLLoader::OnReceiveResponse(
    const ResourceResponseHead& response_head,
    const base::Optional<net::SSLInfo>& ssl_info,
    mojom::DownloadedTempFilePtr downloaded_file) {
  if (!version_->context() || version_->is_redundant()) {
    Finalize(ResourceRequestCompletionStatus(net::ERR_FAILED));
    return;
  }

  // We don't have complete info here, but fill in what we have now.
  // At least we need headers and SSL info.
  auto response_info = std::make_unique<net::HttpResponseInfo>();
  response_info->headers = response_head.headers;
  if (ssl_info.has_value())
    response_info->ssl_info = *ssl_info;
  response_info->was_fetched_via_spdy = response_head.was_fetched_via_spdy;
  response_info->was_alpn_negotiated = response_head.was_alpn_negotiated;
  response_info->alpn_negotiated_protocol =
      response_head.alpn_negotiated_protocol;
  response_info->connection_info = response_head.connection_info;
  response_info->socket_address = response_head.socket_address;

  // TODO(nhiroki): Check the response code.

  // TODO(nhiroki): Check the path restriction.
  // (See ServiceWorkerWriteToCacheJob::CheckPathRestriction())

  // TODO(nhiroki): Check the SSL certificate.

  // TODO(nhiroki): Check the MIME type.

  // TODO(nhiroki): Notify the embedded worker whether the network was accessed
  // for script load.
  // (See EmbeddedWorkerInstance::OnNetWorkAccessedForScriptLoad)

  if (resource_type_ == RESOURCE_TYPE_SERVICE_WORKER)
    version_->SetMainScriptHttpResponseInfo(*response_info);

  WriteHeaders(
      base::MakeRefCounted<HttpResponseInfoIOBuffer>(response_info.release()));

  forwarding_client_->OnReceiveResponse(response_head, ssl_info,
                                        std::move(downloaded_file));
}

void ServiceWorkerScriptURLLoader::OnReceiveRedirect(
    const net::RedirectInfo& redirect_info,
    const ResourceResponseHead& response_head) {
  forwarding_client_->OnReceiveRedirect(redirect_info, response_head);
}

void ServiceWorkerScriptURLLoader::OnDataDownloaded(int64_t data_len,
                                                    int64_t encoded_data_len) {
  forwarding_client_->OnDataDownloaded(data_len, encoded_data_len);
}

void ServiceWorkerScriptURLLoader::OnUploadProgress(
    int64_t current_position,
    int64_t total_size,
    OnUploadProgressCallback ack_callback) {
  forwarding_client_->OnUploadProgress(current_position, total_size,
                                       std::move(ack_callback));
}

void ServiceWorkerScriptURLLoader::OnReceiveCachedMetadata(
    const std::vector<uint8_t>& data) {
  forwarding_client_->OnReceiveCachedMetadata(data);
}

void ServiceWorkerScriptURLLoader::OnTransferSizeUpdated(
    int32_t transfer_size_diff) {
  forwarding_client_->OnTransferSizeUpdated(transfer_size_diff);
}

void ServiceWorkerScriptURLLoader::OnStartLoadingResponseBody(
    mojo::ScopedDataPipeConsumerHandle consumer) {
  // Create a pair of the consumer and producer for forwarding.
  mojo::ScopedDataPipeConsumerHandle forwarding_consumer;
  if (mojo::CreateDataPipe(nullptr, &forwarding_producer_,
                           &forwarding_consumer) != MOJO_RESULT_OK) {
    Finalize(ResourceRequestCompletionStatus(net::ERR_FAILED));
    return;
  }

  // Pass the consumer handle for forwarding the response to the controller in
  // the renderer process.
  forwarding_client_->OnStartLoadingResponseBody(
      std::move(forwarding_consumer));

  network_consumer_ = std::move(consumer);

  if (!did_write_headers_) {
    // Wait until the headers are written in the storage because the cache
    // writer cannot write the headers and data in parallel.
    return;
  }
  StartHandleWatcher();
}

void ServiceWorkerScriptURLLoader::StartHandleWatcher() {
  DCHECK(network_consumer_.is_valid());
  DCHECK(did_write_headers_);
  watcher_.Watch(network_consumer_.get(),
                 MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED,
                 base::Bind(&ServiceWorkerScriptURLLoader::OnDataAvailable,
                            weak_factory_.GetWeakPtr()));
  watcher_.ArmOrNotify();
}

void ServiceWorkerScriptURLLoader::OnComplete(
    const ResourceRequestCompletionStatus& status) {
  if (status.error_code != net::OK) {
    Finalize(status);
    return;
  }
  // Finalize() will be called after the headers and data wre written in the
  // storage.
  completion_closure_.Run();
}

void ServiceWorkerScriptURLLoader::InitializeScriptCacheWriter() {
  // |incumbent_resource_id| is valid if the incumbent service worker exists and
  // it's required to do the byte-for-byte check.
  int64_t incumbent_resource_id = kInvalidServiceWorkerResourceId;
  if (resource_type_ == RESOURCE_TYPE_SERVICE_WORKER) {
    scoped_refptr<ServiceWorkerRegistration> registration =
        version_->context()->GetLiveRegistration(version_->registration_id());
    DCHECK(registration);
    ServiceWorkerVersion* stored_version = registration->waiting_version()
                                               ? registration->waiting_version()
                                               : registration->active_version();
    // |pause_after_download()| indicates the version is required to do the
    // byte-for-byte check.
    if (stored_version && stored_version->script_url() == request_url_ &&
        version_->pause_after_download()) {
      incumbent_resource_id =
          stored_version->script_cache_map()->LookupResourceId(request_url_);
    }
  }

  // Create response readers only when we have to do the byte-for-byte check.
  std::unique_ptr<ServiceWorkerResponseReader> compare_reader;
  std::unique_ptr<ServiceWorkerResponseReader> copy_reader;
  ServiceWorkerStorage* storage = version_->context()->storage();
  if (incumbent_resource_id != kInvalidServiceWorkerResourceId) {
    compare_reader = storage->CreateResponseReader(incumbent_resource_id);
    copy_reader = storage->CreateResponseReader(incumbent_resource_id);
  }
  cache_writer_ = base::MakeUnique<ServiceWorkerCacheWriter>(
      std::move(compare_reader), std::move(copy_reader),
      storage->CreateResponseWriter(resource_id_));
}

void ServiceWorkerScriptURLLoader::WriteHeaders(
    scoped_refptr<HttpResponseInfoIOBuffer> info_buffer) {
  net::Error error = cache_writer_->MaybeWriteHeaders(
      info_buffer.get(),
      base::Bind(&ServiceWorkerScriptURLLoader::OnWriteHeadersComplete,
                 weak_factory_.GetWeakPtr()));
  if (error != net::ERR_IO_PENDING && error != net::OK) {
    Finalize(ResourceRequestCompletionStatus(net::ERR_FAILED));
    return;
  }
}

void ServiceWorkerScriptURLLoader::OnWriteHeadersComplete(net::Error error) {
  DCHECK_NE(net::ERR_IO_PENDING, error);
  if (error != net::OK) {
    Finalize(ResourceRequestCompletionStatus(error));
    return;
  }
  did_write_headers_ = true;
  if (!network_consumer_.is_valid()) {
    // Wait until the network consumer handle is ready to read.
    return;
  }
  StartHandleWatcher();
}

void ServiceWorkerScriptURLLoader::OnDataAvailable(MojoResult) {
  DCHECK(network_consumer_.is_valid());
  DCHECK(!pending_read_);
  uint32_t available = 0;
  MojoResult result = network::MojoToNetPendingBuffer::BeginRead(
      &network_consumer_, &pending_read_, &available);
  switch (result) {
    case MOJO_RESULT_FAILED_PRECONDITION:
      // Closed by peer. This indicates all the data from the network service
      // are read or there is an error. In the error case, the reason is
      // notified via OnComplete().
      completion_closure_.Run();
      return;
    case MOJO_RESULT_SHOULD_WAIT:
      watcher_.ArmOrNotify();
      return;
    case MOJO_RESULT_OK:
      break;
    default:
      // TODO(nhiroki): Currently we handle a few limited cases. Audit other
      // cases.
      NOTREACHED();
      return;
  }

  // Read the received data to |buffer|.
  // TODO(nhiroki): Tweak the buffer size.
  const size_t kBufferSize = std::numeric_limits<size_t>::max();
  size_t bytes_to_be_read = std::min<size_t>(kBufferSize, available);
  auto buffer = base::MakeRefCounted<network::MojoToNetIOBuffer>(
      pending_read_.get(), bytes_to_be_read);

  WriteData(std::move(buffer), bytes_to_be_read);
}

void ServiceWorkerScriptURLLoader::WriteData(
    scoped_refptr<net::IOBuffer> buffer,
    size_t available_bytes) {
  net::Error error = cache_writer_->MaybeWriteData(
      buffer.get(), available_bytes,
      base::Bind(&ServiceWorkerScriptURLLoader::OnWriteDataComplete,
                 weak_factory_.GetWeakPtr(), available_bytes));
  if (error == net::ERR_IO_PENDING) {
    // OnWriteDataComplete() will be called.
    return;
  }
  OnWriteDataComplete(available_bytes, error);
}

void ServiceWorkerScriptURLLoader::OnWriteDataComplete(size_t bytes_written,
                                                       net::Error error) {
  DCHECK_NE(net::ERR_IO_PENDING, error);
  if (error != net::OK) {
    Finalize(ResourceRequestCompletionStatus(error));
    return;
  }
  if (pending_read_) {
    pending_read_->CompleteRead(bytes_written);
    // Get the consumer handle from a previous read operation if we have one.
    network_consumer_ = pending_read_->ReleaseHandle();
    pending_read_ = nullptr;
  }
  watcher_.ArmOrNotify();
}

void ServiceWorkerScriptURLLoader::OnWriteHeadersAndDataComplete() {
  Finalize(ResourceRequestCompletionStatus(net::OK));
}

void ServiceWorkerScriptURLLoader::Finalize(
    const ResourceRequestCompletionStatus& status) {
  net::Error error_code = static_cast<net::Error>(status.error_code);
  int bytes_written = -1;
  std::string status_message;
  if (status.error_code == net::OK) {
    bytes_written = cache_writer_->bytes_written();
    if (!cache_writer_->did_replace()) {
      version_->SetStartWorkerStatusCode(SERVICE_WORKER_ERROR_EXISTS);
      status_message = ServiceWorkerWriteToCacheJob::kIdenticalScriptError;
    }
  } else {
    // AddMessageConsole must be called before notifying that an error occurred
    // because the worker stops soon after receiving the error response.
    // TODO(nhiroki): Provide more accurate error message.
    version_->embedded_worker()->AddMessageToConsole(
        blink::WebConsoleMessage::kLevelError, kFetchScriptError);
    // TODO(nhiroki): Set |status_message| to notify the exact error reason.
  }
  version_->script_cache_map()->NotifyFinishedCaching(
      request_url_, bytes_written, error_code, status_message);

  // TODO(nhiroki): Record ServiceWorkerMetrics::CountWriteResponseResult().
  forwarding_client_->OnComplete(status);

  network_client_binding_.Close();
  network_loader_.reset();
  watcher_.Cancel();
  cache_writer_.reset();
  pending_read_ = nullptr;
}

}  // namespace content
