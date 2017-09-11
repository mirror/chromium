// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/service_worker/service_worker_script_url_loader.h"

#include <memory>
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
    : url_(resource_request.url),
      network_client_binding_(this),
      forwarding_client_(std::move(client)),
      version_(version),
      watcher_(FROM_HERE, mojo::SimpleWatcher::ArmingPolicy::MANUAL),
      weak_factory_(this) {
  DCHECK(version_->context());
  DCHECK_EQ(ServiceWorkerVersion::NEW, version->status());

  mojom::URLLoaderClientPtr network_client;
  network_client_binding_.Bind(mojo::MakeRequest(&network_client));

  ServiceWorkerStorage* storage = version_->context()->storage();
  int resource_id = storage->NewResourceId();
  // TODO(nhiroki): Handle an error case.
  // if (resource_id == kInvalidServiceWorkerResourceId) {}

  // |incumbent_resource_id| is valid if the incumbent service worker exists and
  // it's required to do the byte-for-byte check.
  int64_t incumbent_resource_id = kInvalidServiceWorkerResourceId;
  if (resource_request.resource_type == RESOURCE_TYPE_SERVICE_WORKER) {
    scoped_refptr<ServiceWorkerRegistration> registration =
        version_->context()->GetLiveRegistration(version->registration_id());
    DCHECK(registration);
    ServiceWorkerVersion* stored_version = registration->waiting_version()
                                               ? registration->waiting_version()
                                               : registration->active_version();
    // |pause_after_download()| indicates the version is required to do the
    // byte-for-byte check.
    if (stored_version && stored_version->script_url() == url_ &&
        version->pause_after_download()) {
      incumbent_resource_id =
          stored_version->script_cache_map()->LookupResourceId(url_);
    }
  }

  // Create response readers only when we have to do the byte-for-byte check.
  std::unique_ptr<ServiceWorkerResponseReader> compare_reader;
  std::unique_ptr<ServiceWorkerResponseReader> copy_reader;
  if (incumbent_resource_id != kInvalidServiceWorkerResourceId) {
    compare_reader = storage->CreateResponseReader(incumbent_resource_id);
    copy_reader = storage->CreateResponseReader(incumbent_resource_id);
  }
  cache_writer_ = base::MakeUnique<ServiceWorkerCacheWriter>(
      std::move(compare_reader), std::move(copy_reader),
      storage->CreateResponseWriter(resource_id));

  version_->script_cache_map()->NotifyStartedCaching(url_, resource_id);
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
    OnComplete(ResourceRequestCompletionStatus(net::ERR_FAILED));
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

  version_->SetMainScriptHttpResponseInfo(*response_info);

  auto info_buffer =
      base::MakeRefCounted<HttpResponseInfoIOBuffer>(response_info.release());
  net::Error error = cache_writer_->MaybeWriteHeaders(
      info_buffer.get(),
      base::Bind(&ServiceWorkerScriptURLLoader::OnWriteHeadersComplete,
                 weak_factory_.GetWeakPtr(), response_head, ssl_info,
                 base::Passed(std::move(downloaded_file))));
  if (error == net::ERR_IO_PENDING) {
    // OnWriteHeadersComplete() will be called.
    return;
  }
  if (error == net::OK) {
    // TODO(nhiroki): How do we call OnWriteDataComplete()? |downloaded_file| is
    // already passed.
    return;
  }
  OnComplete(ResourceRequestCompletionStatus(net::ERR_FAILED));
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
  mojo::ScopedDataPipeConsumerHandle forwarding_consumer;
  if (mojo::CreateDataPipe(nullptr, &forwarding_producer_,
                           &forwarding_consumer) != MOJO_RESULT_OK) {
    // TODO(nhiroki): Record the result.
    OnComplete(ResourceRequestCompletionStatus(net::ERR_FAILED));
    return;
  }

  consumer_ = std::move(consumer);
  watcher_.Watch(consumer_.get(),
                 MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED,
                 base::Bind(&ServiceWorkerScriptURLLoader::OnDataAvailable,
                            weak_factory_.GetWeakPtr()));
  watcher_.ArmOrNotify();

  forwarding_client_->OnStartLoadingResponseBody(
      std::move(forwarding_consumer));
}

void ServiceWorkerScriptURLLoader::OnComplete(
    const ResourceRequestCompletionStatus& status) {
  int size = -1;
  if (status.error_code != net::OK) {
    // AddMessageConsole must be called before notifying that an error occurred
    // because the worker stops soon after receiving the error response.
    // TODO(nhiroki): Provide more accurate error message.
    version_->embedded_worker()->AddMessageToConsole(
        blink::WebConsoleMessage::kLevelError, kFetchScriptError);
  } else {
    size = cache_writer_->bytes_written();
  }

  if (status.error_code == net::OK && !cache_writer_->did_replace()) {
    version_->SetStartWorkerStatusCode(SERVICE_WORKER_ERROR_EXISTS);
    version_->script_cache_map()->NotifyFinishedCaching(
        url_, size, ServiceWorkerWriteToCacheJob::kIdenticalScriptError,
        std::string());
  } else {
    // TODO(nhiroki): Provide more accurate error message.
    version_->script_cache_map()->NotifyFinishedCaching(
        url_, size, static_cast<net::Error>(status.error_code),
        kFetchScriptError);
  }

  // TODO(nhiroki): Record ServiceWorkerMetrics::CountWriteResponseResult().
  forwarding_client_->OnComplete(status);

  network_client_binding_.Close();
  network_loader_.reset();
  watcher_.Cancel();
  cache_writer_.reset();
  pending_read_ = nullptr;
}

void ServiceWorkerScriptURLLoader::OnWriteHeadersComplete(
    const ResourceResponseHead& response_head,
    const base::Optional<net::SSLInfo>& ssl_info,
    mojom::DownloadedTempFilePtr downloaded_file,
    net::Error error) {
  DCHECK_NE(net::ERR_IO_PENDING, error);
  if (error != net::OK) {
    OnComplete(ResourceRequestCompletionStatus(error));
    return;
  }
  forwarding_client_->OnReceiveResponse(response_head, ssl_info,
                                        std::move(downloaded_file));
}

// REFERENCE: appcache_update_url_loader_request.cc
// REFERENCE: service_worker_write_to_cache_job.cc
void ServiceWorkerScriptURLLoader::OnDataAvailable(MojoResult) {
  // Get the |consumer_| from a previous read operation if we have one.
  if (pending_read_) {
    DCHECK(pending_read_->IsComplete());
    consumer_ = pending_read_->ReleaseHandle();
    pending_read_ = nullptr;
  }

  uint32_t available = 0;
  MojoResult result = network::MojoToNetPendingBuffer::BeginRead(
      &consumer_, &pending_read_, &available);

  switch (result) {
    case MOJO_RESULT_BUSY:
    case MOJO_RESULT_INVALID_ARGUMENT:
      NOTREACHED();
      return;
    case MOJO_RESULT_FAILED_PRECONDITION:
      // Closed by peer.
      OnComplete(ResourceRequestCompletionStatus(net::ERR_FAILED));
      return;
    case MOJO_RESULT_SHOULD_WAIT:
      watcher_.ArmOrNotify();
      return;
    case MOJO_RESULT_OK:
      break;
    default:
      // TODO(nhiroki)
      NOTREACHED();
      return;
  }

  size_t bytes_to_be_read =
      std::min<size_t>(std::numeric_limits<size_t>::max(), available);
  auto buffer = base::MakeRefCounted<network::MojoToNetIOBuffer>(
      pending_read_.get(), bytes_to_be_read);

  net::Error error = cache_writer_->MaybeWriteData(
      buffer.get(), bytes_to_be_read,
      base::Bind(&ServiceWorkerScriptURLLoader::OnWriteDataComplete,
                 weak_factory_.GetWeakPtr()));
  if (error == net::ERR_IO_PENDING) {
    // OnWriteDataComplete() will be called.
    return;
  }
  if (error == net::OK) {
    OnWriteDataComplete(net::OK);
    return;
  }
  OnComplete(ResourceRequestCompletionStatus(error));
}

void ServiceWorkerScriptURLLoader::OnWriteDataComplete(net::Error error) {
  DCHECK_NE(net::ERR_IO_PENDING, error);
  if (error != net::OK) {
    OnComplete(ResourceRequestCompletionStatus(error));
    return;
  }
  if (pending_read_) {
    // Continue to read.
    OnDataAvailable(MOJO_RESULT_OK);
    return;
  }
}

}  // namespace content
