// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/appcache/appcache_subresource_url_factory.h"

#include "base/bind.h"
#include "base/logging.h"
#include "content/browser/appcache/appcache_host.h"
#include "content/browser/appcache/appcache_request_handler.h"
#include "content/browser/appcache/appcache_url_loader_job.h"
#include "content/browser/appcache/appcache_url_loader_request.h"
#include "content/browser/url_loader_factory_getter.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/resource_request.h"
#include "content/public/common/url_loader_factory.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "mojo/public/cpp/bindings/interface_ptr.h"
#include "net/traffic_annotation/network_traffic_annotation.h"

namespace content {

AppCacheSubresourceURLFactory::SubresourceLoader::SubresourceLoader(
    mojom::URLLoaderRequest url_loader_request,
    int32_t routing_id,
    int32_t request_id,
    uint32_t options,
    const ResourceRequest& request,
    mojom::URLLoaderClientPtr client,
    const net::MutableNetworkTrafficAnnotationTag& annotation,
    base::WeakPtr<AppCacheHost> appcache_host,
    scoped_refptr<URLLoaderFactoryGetter> net_factory_getter)
    : remote_binding_(this, std::move(url_loader_request)),
      remote_client_(std::move(client)),
      request_(request),
      routing_id_(routing_id),
      request_id_(request_id),
      options_(options),
      traffic_annotation_(annotation),
      network_loader_factory_(std::move(net_factory_getter)),
      local_client_binding_(this),
      host_(appcache_host),
      weak_factory_(this) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  remote_binding_.set_connection_error_handler(base::Bind(
      &SubresourceLoader::OnConnectionError, base::Unretained(this)));
  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindOnce(&SubresourceLoader::Start, weak_factory_.GetWeakPtr()));
}

AppCacheSubresourceURLFactory::SubresourceLoader::~SubresourceLoader() {}

void AppCacheSubresourceURLFactory::SubresourceLoader::SetHandlerForTesting(
    std::unique_ptr<AppCacheRequestHandler> handler) {
  handler_ = std::move(handler);
}

void AppCacheSubresourceURLFactory::SubresourceLoader::OnConnectionError() {
  delete this;
}

void AppCacheSubresourceURLFactory::SubresourceLoader::Start() {
  if (!host_) {
    remote_client_->OnComplete(
        ResourceRequestCompletionStatus(net::ERR_FAILED));
    return;
  }
  if (!handler_) {  // It may be non-null if SetHandlerForTesting was called.
    handler_ = host_->CreateRequestHandler(
        AppCacheURLLoaderRequest::Create(request_), request_.resource_type,
        request_.should_reset_appcache);
    if (!handler_) {
      CreateAndStartNetworkLoader();
      return;
    }
  }
  handler_->MaybeCreateSubresourceLoader(
      request_, base::BindOnce(&SubresourceLoader::ContinueStart,
                               weak_factory_.GetWeakPtr()));
}

void AppCacheSubresourceURLFactory::SubresourceLoader::ContinueStart(
    StartLoaderCallback start_function) {
  if (start_function)
    CreateAndStartAppCacheLoader(std::move(start_function));
  else
    CreateAndStartNetworkLoader();
}

void AppCacheSubresourceURLFactory::SubresourceLoader::
    CreateAndStartAppCacheLoader(StartLoaderCallback start_function) {
  DCHECK(!appcache_loader_) << "only expected to be called onced";
  DCHECK(start_function);

  // Disconnect from the network loader first.
  local_client_binding_.Close();
  network_loader_ = nullptr;

  mojom::URLLoaderClientPtr client_ptr;
  local_client_binding_.Bind(mojo::MakeRequest(&client_ptr));
  std::move(start_function)
      .Run(mojo::MakeRequest(&appcache_loader_), std::move(client_ptr));
}

void AppCacheSubresourceURLFactory::SubresourceLoader::
    CreateAndStartNetworkLoader() {
  DCHECK(!appcache_loader_);
  mojom::URLLoaderClientPtr client_ptr;
  local_client_binding_.Bind(mojo::MakeRequest(&client_ptr));
  network_loader_factory_->GetNetworkFactory()->get()->CreateLoaderAndStart(
      mojo::MakeRequest(&network_loader_), routing_id_, request_id_, options_,
      request_, std::move(client_ptr), traffic_annotation_);
  if (has_set_priority_)
    network_loader_->SetPriority(priority_, intra_priority_value_);
  if (has_paused_reading_)
    network_loader_->PauseReadingBodyFromNet();
}

// mojom::URLLoader implementation
// Called by the remote client in the renderer.
void AppCacheSubresourceURLFactory::SubresourceLoader::FollowRedirect() {
  if (!network_loader_)
    return;  // This is a bad message
  if (!handler_) {
    network_loader_->FollowRedirect();
    return;
  }
  DCHECK(network_loader_);
  DCHECK(!appcache_loader_);
  handler_->MaybeFollowSubresourceRedirect(
      redirect_info_, base::BindOnce(&SubresourceLoader::ContinueFollowRedirect,
                                     weak_factory_.GetWeakPtr()));
}

void AppCacheSubresourceURLFactory::SubresourceLoader::ContinueFollowRedirect(
    StartLoaderCallback start_function) {
  if (start_function)
    CreateAndStartAppCacheLoader(std::move(start_function));
  else
    network_loader_->FollowRedirect();
}

void AppCacheSubresourceURLFactory::SubresourceLoader::SetPriority(
    net::RequestPriority priority,
    int32_t intra_priority_value) {
  has_set_priority_ = true;
  priority_ = priority;
  intra_priority_value_ = intra_priority_value;
  if (network_loader_)
    network_loader_->SetPriority(priority, intra_priority_value);
}

void AppCacheSubresourceURLFactory::SubresourceLoader::
    PauseReadingBodyFromNet() {
  has_paused_reading_ = true;
  if (network_loader_)
    network_loader_->PauseReadingBodyFromNet();
}

void AppCacheSubresourceURLFactory::SubresourceLoader::
    ResumeReadingBodyFromNet() {
  has_paused_reading_ = false;
  if (network_loader_)
    network_loader_->ResumeReadingBodyFromNet();
}

void AppCacheSubresourceURLFactory::SubresourceLoader::OnReceiveResponse(
    const ResourceResponseHead& response_head,
    const base::Optional<net::SSLInfo>& ssl_info,
    mojom::DownloadedTempFilePtr downloaded_file) {
  // Don't MaybeFallback for appcache produced responses.
  if (appcache_loader_ || !handler_) {
    remote_client_->OnReceiveResponse(response_head, ssl_info,
                                      std::move(downloaded_file));
    return;
  }

  did_receive_network_response_ = true;
  handler_->MaybeFallbackForSubresourceResponse(
      response_head,
      base::BindOnce(&SubresourceLoader::ContinueOnReceiveResponse,
                     weak_factory_.GetWeakPtr(), response_head, ssl_info,
                     std::move(downloaded_file)));
}

void AppCacheSubresourceURLFactory::SubresourceLoader::
    ContinueOnReceiveResponse(const ResourceResponseHead& response_head,
                              const base::Optional<net::SSLInfo>& ssl_info,
                              mojom::DownloadedTempFilePtr downloaded_file,
                              StartLoaderCallback start_function) {
  if (start_function) {
    CreateAndStartAppCacheLoader(std::move(start_function));
  } else {
    remote_client_->OnReceiveResponse(response_head, ssl_info,
                                      std::move(downloaded_file));
  }
}

void AppCacheSubresourceURLFactory::SubresourceLoader::OnReceiveRedirect(
    const net::RedirectInfo& redirect_info,
    const ResourceResponseHead& response_head) {
  DCHECK(network_loader_) << "appcache loader does not produce redirects";
  if (!redirect_limit_--) {
    OnComplete(ResourceRequestCompletionStatus(net::ERR_TOO_MANY_REDIRECTS));
    return;
  }
  if (!handler_) {
    remote_client_->OnReceiveRedirect(redirect_info_, response_head);
    return;
  }
  redirect_info_ = redirect_info;
  handler_->MaybeFallbackForSubresourceRedirect(
      redirect_info,
      base::BindOnce(&SubresourceLoader::ContinueOnReceiveRedirect,
                     weak_factory_.GetWeakPtr(), response_head));
}

void AppCacheSubresourceURLFactory::SubresourceLoader::
    ContinueOnReceiveRedirect(const ResourceResponseHead& response_head,
                              StartLoaderCallback start_function) {
  if (start_function)
    CreateAndStartAppCacheLoader(std::move(start_function));
  else
    remote_client_->OnReceiveRedirect(redirect_info_, response_head);
}

void AppCacheSubresourceURLFactory::SubresourceLoader::OnDataDownloaded(
    int64_t data_len,
    int64_t encoded_data_len) {
  remote_client_->OnDataDownloaded(data_len, encoded_data_len);
}

void AppCacheSubresourceURLFactory::SubresourceLoader::OnUploadProgress(
    int64_t current_position,
    int64_t total_size,
    OnUploadProgressCallback ack_callback) {
  remote_client_->OnUploadProgress(current_position, total_size,
                                   std::move(ack_callback));
}

void AppCacheSubresourceURLFactory::SubresourceLoader::OnReceiveCachedMetadata(
    const std::vector<uint8_t>& data) {
  remote_client_->OnReceiveCachedMetadata(data);
}

void AppCacheSubresourceURLFactory::SubresourceLoader::OnTransferSizeUpdated(
    int32_t transfer_size_diff) {
  remote_client_->OnTransferSizeUpdated(transfer_size_diff);
}

void AppCacheSubresourceURLFactory::SubresourceLoader::
    OnStartLoadingResponseBody(mojo::ScopedDataPipeConsumerHandle body) {
  remote_client_->OnStartLoadingResponseBody(std::move(body));
}

void AppCacheSubresourceURLFactory::SubresourceLoader::OnComplete(
    const ResourceRequestCompletionStatus& status) {
  if (!network_loader_ || !handler_ || did_receive_network_response_ ||
      status.error_code == net::OK) {
    remote_client_->OnComplete(status);
    return;
  }
  handler_->MaybeFallbackForSubresourceResponse(
      ResourceResponseHead(),
      base::BindOnce(&SubresourceLoader::ContinueOnComplete,
                     weak_factory_.GetWeakPtr(), status));
}

void AppCacheSubresourceURLFactory::SubresourceLoader::ContinueOnComplete(
    const ResourceRequestCompletionStatus& status,
    StartLoaderCallback start_function) {
  if (start_function)
    CreateAndStartAppCacheLoader(std::move(start_function));
  else
    remote_client_->OnComplete(status);
}

// Implements the URLLoaderFactory mojom for AppCache requests.
AppCacheSubresourceURLFactory::AppCacheSubresourceURLFactory(
    URLLoaderFactoryGetter* default_url_loader_factory_getter,
    base::WeakPtr<AppCacheHost> host)
    : default_url_loader_factory_getter_(default_url_loader_factory_getter),
      appcache_host_(host),
      weak_factory_(this) {
  bindings_.set_connection_error_handler(
      base::Bind(&AppCacheSubresourceURLFactory::OnConnectionError,
                 base::Unretained(this)));
}

AppCacheSubresourceURLFactory::~AppCacheSubresourceURLFactory() {}

// static
void AppCacheSubresourceURLFactory::CreateURLLoaderFactory(
    URLLoaderFactoryGetter* default_url_loader_factory_getter,
    base::WeakPtr<AppCacheHost> host,
    mojom::URLLoaderFactoryPtr* loader_factory) {
  DCHECK(host.get());
  // This instance is effectively reference counted by the number of pipes open
  // to it and will get deleted when all clients drop their connections.
  // Please see OnConnectionError() for details.
  auto* impl = new AppCacheSubresourceURLFactory(
      default_url_loader_factory_getter, host);
  impl->Clone(mojo::MakeRequest(loader_factory));

  // Save the factory in the host to ensure that we don't create it again when
  // the cache is selected, etc.
  host->SetAppCacheSubresourceFactory(impl);
}

void AppCacheSubresourceURLFactory::CreateLoaderAndStart(
    mojom::URLLoaderRequest url_loader_request,
    int32_t routing_id,
    int32_t request_id,
    uint32_t options,
    const ResourceRequest& request,
    mojom::URLLoaderClientPtr client,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  new SubresourceLoader(std::move(url_loader_request), routing_id, request_id,
                        options, request, std::move(client), traffic_annotation,
                        appcache_host_, default_url_loader_factory_getter_);
}

void AppCacheSubresourceURLFactory::Clone(
    mojom::URLLoaderFactoryRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

base::WeakPtr<AppCacheSubresourceURLFactory>
AppCacheSubresourceURLFactory::GetWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

void AppCacheSubresourceURLFactory::OnConnectionError() {
  if (bindings_.empty())
    delete this;
}

}  // namespace content
