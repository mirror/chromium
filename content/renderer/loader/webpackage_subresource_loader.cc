// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/loader/webpackage_subresource_loader.h"

#include "base/numerics/safe_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "content/public/common/resource_request.h"
#include "content/public/renderer/child_url_loader_factory_getter.h"
#include "net/http/http_util.h"

namespace content {

namespace {

// This class destroys itself when the Mojo pipe bound to this object
// is dropped, or the Mojo request is passed over to the network service.
class SubresourceLoader : public mojom::URLLoader {
 public:
  SubresourceLoader(
      mojom::URLLoaderRequest loader_request,
      scoped_refptr<WebPackageSubresourceManagerImpl> subresource_manager,
      scoped_refptr<ChildURLLoaderFactoryGetter> loader_factory_getter,
      int32_t routing_id,
      int32_t request_id,
      uint32_t options,
      const ResourceRequest& resource_request,
      mojom::URLLoaderClientPtr loader_client,
      const net::MutableNetworkTrafficAnnotationTag& traffic_annotation,
      bool allow_fallback)
      : binding_(this, std::move(loader_request)),
        loader_client_(std::move(loader_client)),
        subresource_manager_(std::move(subresource_manager)),
        loader_factory_getter_(std::move(loader_factory_getter)),
        resource_request_(resource_request),
        weak_factory_(this) {
    binding_.set_connection_error_handler(base::Bind(
        &SubresourceLoader::OnConnectionError, base::Unretained(this)));
    subresource_manager_->GetResource(resource_request,
                         base::BindOnce(&SubresourceLoader::OnFoundResource,
                                        weak_factory_.GetWeakPtr(), routing_id,
                                        request_id, options, resource_request,
                                        traffic_annotation, allow_fallback),
                         base::BindOnce(&SubresourceLoader::OnComplete,
                                        weak_factory_.GetWeakPtr()));
  }

 private:
  void OnFoundResource(
      int32_t routing_id,
      int32_t request_id,
      uint32_t options,
      const ResourceRequest& resource_request,
      const net::MutableNetworkTrafficAnnotationTag& traffic_annotation,
      bool allow_fallback,
      int error_code,
      const ResourceResponseHead& response_head,
      mojo::ScopedDataPipeConsumerHandle body) {
    if (error_code == net::ERR_FILE_NOT_FOUND) {
      if (allow_fallback) {
        // Let the NetworkFactory handle it.
        loader_factory_getter_->GetNetworkLoaderFactory()
            ->CreateLoaderAndStart(binding_.Unbind(), routing_id, request_id,
                                   options, resource_request,
                                   std::move(loader_client_),
                                   traffic_annotation);
        base::ThreadTaskRunnerHandle::Get()->DeleteSoon(FROM_HERE, this);
        return;
      }
      // TODO(kinuko): Maybe we don't want to generate 404 not found here,
      // we may want auto-reload on navigation.
      std::string buf("HTTP/1.1 404 Not Found\r\n");
      ResourceResponseHead response_head;
      response_head.headers = new net::HttpResponseHeaders(
          net::HttpUtil::AssembleRawHeaders(buf.c_str(), buf.size()));
      loader_client_->OnReceiveResponse(response_head, base::nullopt,
                                        nullptr /* download_file */);
      loader_client_->OnComplete(network::URLLoaderCompletionStatus(net::OK));
      return;
    }
    if (error_code != net::OK) {
      loader_client_->OnComplete(
          network::URLLoaderCompletionStatus(error_code));
      return;
    }
    loader_client_->OnReceiveResponse(response_head, base::nullopt,
                                      nullptr /* download_file */);
    loader_client_->OnStartLoadingResponseBody(std::move(body));
  }
  void OnComplete(const network::URLLoaderCompletionStatus& status) {
    loader_client_->OnComplete(status);
  }
  void OnConnectionError() {
    base::ThreadTaskRunnerHandle::Get()->DeleteSoon(FROM_HERE, this);
  }

  void FollowRedirect() override {}
  void SetPriority(net::RequestPriority priority,
                   int32_t intra_priority_value) override {}
  void PauseReadingBodyFromNet() override {}
  void ResumeReadingBodyFromNet() override {}

  mojo::Binding<mojom::URLLoader> binding_;
  mojom::URLLoaderClientPtr loader_client_;

  scoped_refptr<WebPackageSubresourceManagerImpl> subresource_manager_;
  scoped_refptr<ChildURLLoaderFactoryGetter> loader_factory_getter_;

  ResourceRequest resource_request_;

  base::WeakPtrFactory<SubresourceLoader> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(SubresourceLoader);
};

}  // namespace

WebPackageSubresourceLoaderFactory::WebPackageSubresourceLoaderFactory(
    scoped_refptr<WebPackageSubresourceManagerImpl> subresource_manager,
    scoped_refptr<ChildURLLoaderFactoryGetter> loader_factory_getter)
    : subresource_manager_(std::move(subresource_manager)),
      loader_factory_getter_(std::move(loader_factory_getter)),
      weak_factory_(this) {
  bindings_.set_connection_error_handler(base::Bind(
      &WebPackageSubresourceLoaderFactory::OnConnectionError, base::Unretained(
          this)));
}

WebPackageSubresourceLoaderFactory::~WebPackageSubresourceLoaderFactory() {
  // LOG(ERROR) << "** WebPackageSubresourceLoaderFactory:: DTOR";
}

void WebPackageSubresourceLoaderFactory::CreateLoaderAndStart(
    mojom::URLLoaderRequest loader_request,
    int32_t routing_id,
    int32_t request_id,
    uint32_t options,
    const ResourceRequest& resource_request,
    mojom::URLLoaderClientPtr loader_client,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation) {
  LOG(ERROR) << "** CreateWebPackageSubresourceLoader: "
             << resource_request.url;
  new SubresourceLoader(std::move(loader_request), subresource_manager_,
                        loader_factory_getter_, routing_id, request_id,
                        options, resource_request, std::move(loader_client),
                        traffic_annotation, allow_fallback_);
}

void WebPackageSubresourceLoaderFactory::Clone(
    mojom::URLLoaderFactoryRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

}  // namespace content
