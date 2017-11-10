// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "content/browser/loader/navigation_mojo_response_url_loader_factory.h"

#include "base/macros.h"
#include "content/browser/loader/resource_dispatcher_host_impl.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/navigation_ui_data.h"

namespace content {

namespace {

class NavigationMojoResponseFactory : public mojom::URLLoaderFactory {
 public:
  NavigationMojoResponseFactory(
      ResourceContext* resource_context,
      net::URLRequestContext* request_context,
      storage::FileSystemContext* upload_file_system_context,
      const NavigationRequestInfo& info,
      std::unique_ptr<NavigationUIData> navigation_ui_data,
      ServiceWorkerNavigationHandleCore* service_worker_handle_core,
      AppCacheNavigationHandleCore* appcache_handle_core)
      : resource_context_(resource_context),
        request_context_(request_context),
        upload_file_system_context_(upload_file_system_context),
        info_(info),
        navigation_ui_data_(std::move(navigation_ui_data)),
        service_worker_handle_core_(service_worker_handle_core),
        appcache_handle_core_(appcache_handle_core) {}

  // mojom::URLLoaderFactory implementation
  void CreateLoaderAndStart(mojom::URLLoaderRequest url_loader,
                            int32_t /*routing_id*/,
                            int32_t /*request_id*/,
                            uint32_t /*options*/,
                            const ResourceRequest& /*request*/,
                            mojom::URLLoaderClientPtr url_loader_client,
                            const net::MutableNetworkTrafficAnnotationTag&
                            /*traffic_annotation*/) override {
    DCHECK_CURRENTLY_ON(BrowserThread::IO);
    DCHECK(ResourceDispatcherHostImpl::Get());
    DCHECK(!used_);
    used_ = true;

    ResourceDispatcherHostImpl::Get()->BeginNavigationRequest(
        resource_context_, request_context_, upload_file_system_context_, info_,
        std::move(navigation_ui_data_),
        /*NavigationURLLoaderImplCore = */ nullptr,
        std::move(url_loader_client), std::move(url_loader),
        service_worker_handle_core_, appcache_handle_core_);
  }
  // mojom::URLLoaderFactory implementation
  void Clone(mojom::URLLoaderFactoryRequest request) override { NOTREACHED(); }

 private:
  ResourceContext* resource_context_;
  net::URLRequestContext* request_context_;
  storage::FileSystemContext* upload_file_system_context_;
  const NavigationRequestInfo& info_;
  std::unique_ptr<NavigationUIData> navigation_ui_data_;
  ServiceWorkerNavigationHandleCore* service_worker_handle_core_;
  AppCacheNavigationHandleCore* appcache_handle_core_;

  bool used_ = false;
  DISALLOW_COPY_AND_ASSIGN(NavigationMojoResponseFactory);
};

}  // namespace

std::unique_ptr<mojom::URLLoaderFactory>
CreateNavigationMojoResponseURLLoaderFactory(
    ResourceContext* resource_context,
    net::URLRequestContext* request_context,
    storage::FileSystemContext* upload_file_system_context,
    const NavigationRequestInfo& info,
    std::unique_ptr<NavigationUIData> navigation_ui_data,
    ServiceWorkerNavigationHandleCore* service_worker_handle_core,
    AppCacheNavigationHandleCore* appcache_handle_core) {
  return std::unique_ptr<mojom::URLLoaderFactory>(
      new NavigationMojoResponseFactory(
          resource_context, request_context, upload_file_system_context, info,
          std::move(navigation_ui_data), service_worker_handle_core,
          appcache_handle_core));
}

}  // namespace content
