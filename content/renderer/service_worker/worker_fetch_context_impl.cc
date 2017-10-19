// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/service_worker/worker_fetch_context_impl.h"

#include "base/feature_list.h"
#include "content/child/child_thread_impl.h"
#include "content/child/request_extra_data.h"
#include "content/child/resource_dispatcher.h"
#include "content/child/service_worker/controller_service_worker_connector.h"
#include "content/child/service_worker/service_worker_subresource_loader.h"
#include "content/child/thread_safe_sender.h"
#include "content/child/web_url_loader_impl.h"
#include "content/common/frame_messages.h"
#include "content/common/service_worker/service_worker_utils.h"
#include "content/public/common/content_features.h"
#include "content/public/common/service_names.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/service_manager/public/cpp/connector.h"
#include "third_party/WebKit/public/platform/modules/serviceworker/service_worker_object.mojom.h"

namespace content {

WorkerFetchContextImpl::WorkerFetchContextImpl(
    mojom::ServiceWorkerWorkerClientRequest service_worker_client_request,
    mojom::ServiceWorkerContainerHostPtrInfo service_worker_container_host_info,
    ChildURLLoaderFactoryGetter::Info url_loader_factory_getter_info)
    : binding_(this),
      service_worker_client_request_(std::move(service_worker_client_request)),
      service_worker_container_host_info_(
          std::move(service_worker_container_host_info)),
      url_loader_factory_getter_info_(
          std::move(url_loader_factory_getter_info)),
      thread_safe_sender_(ChildThreadImpl::current()->thread_safe_sender()) {
  ChildThreadImpl::current()->GetConnector()->BindInterface(
      mojom::kBrowserServiceName, mojo::MakeRequest(&blob_registry_ptr_info_));
}

WorkerFetchContextImpl::~WorkerFetchContextImpl() {}

void WorkerFetchContextImpl::InitializeOnWorkerThread(
    const blink::WebURL& context_url,
    scoped_refptr<base::SingleThreadTaskRunner> loading_task_runner) {
  DCHECK(loading_task_runner->RunsTasksInCurrentSequence());
  DCHECK(!resource_dispatcher_);
  DCHECK(!binding_.is_bound());

  context_url_ = context_url;
  resource_dispatcher_ = base::MakeUnique<ResourceDispatcher>(
      nullptr, std::move(loading_task_runner));

  url_loader_factory_getter_ = url_loader_factory_getter_info_.Bind();

  service_worker_container_host_.Bind(
      std::move(service_worker_container_host_info_));

  storage::mojom::BlobRegistryPtr blob_registry_ptr;
  blob_registry_ptr.Bind(std::move(blob_registry_ptr_info_));
  blob_registry_ = base::MakeRefCounted<
      base::RefCountedData<storage::mojom::BlobRegistryPtr>>(
      std::move(blob_registry_ptr));

  if (service_worker_client_request_.is_pending())
    binding_.Bind(std::move(service_worker_client_request_));

  MaybeInitializeSubresourceLoaderFactory();
}

void WorkerFetchContextImpl::MaybeInitializeSubresourceLoaderFactory() {
  if (!ServiceWorkerUtils::IsServicificationEnabled())
    return;
  if (!IsControlledByServiceWorker()) {
    subresource_loader_factory_ = nullptr;
    return;
  }
  if (subresource_loader_factory_.get())
    return;
  mojo::MakeStrongBinding(
      base::MakeUnique<ServiceWorkerSubresourceLoaderFactory>(
          base::MakeRefCounted<ControllerServiceWorkerConnector>(
              service_worker_container_host_.get()),
          url_loader_factory_getter_, context_url_.GetOrigin(), blob_registry_),
      mojo::MakeRequest(&subresource_loader_factory_));
}

std::unique_ptr<blink::WebURLLoader> WorkerFetchContextImpl::CreateURLLoader(
    const blink::WebURLRequest& request,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner) {
  auto loader = CreateServiceWorkerURLLoader(request, task_runner);
  if (loader)
    return loader;
  return base::MakeUnique<content::WebURLLoaderImpl>(
      resource_dispatcher_.get(), std::move(task_runner),
      url_loader_factory_getter_->GetFactoryForURL(request.Url()));
}

std::unique_ptr<blink::WebURLLoader>
WorkerFetchContextImpl::CreateServiceWorkerURLLoader(
    const blink::WebURLRequest& request,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner) {
  // S13nServiceWorker:
  // We need SubresourceLoaderFactory populated in order to
  // create our own URLLoader for subresource loading.
  if (!subresource_loader_factory_)
    return nullptr;

  // S13nServiceWorker:
  // If it's not for HTTP or HTTPS no need to intercept the
  // request.
  if (!GURL(request.Url()).SchemeIsHTTPOrHTTPS())
    return nullptr;

  // S13nServiceWorker:
  // If the service worker mode is not all, no need to intercept the request.
  if (request.GetServiceWorkerMode() !=
      blink::WebURLRequest::ServiceWorkerMode::kAll) {
    return nullptr;
  }

  // S13nServiceWorker:
  // Create our own SubresourceLoader to route the request
  // to the controller ServiceWorker.
  return base::MakeUnique<WebURLLoaderImpl>(resource_dispatcher_.get(),
                                            std::move(task_runner),
                                            subresource_loader_factory_.get());
}

void WorkerFetchContextImpl::WillSendRequest(blink::WebURLRequest& request) {
  RequestExtraData* extra_data = new RequestExtraData();
  extra_data->set_service_worker_provider_id(service_worker_provider_id_);
  extra_data->set_render_frame_id(parent_frame_id_);
  extra_data->set_initiated_in_secure_context(is_secure_context_);
  request.SetExtraData(extra_data);
  request.SetAppCacheHostID(appcache_host_id_);

  if (!IsControlledByServiceWorker() &&
      request.GetServiceWorkerMode() !=
          blink::WebURLRequest::ServiceWorkerMode::kNone) {
    request.SetServiceWorkerMode(
        blink::WebURLRequest::ServiceWorkerMode::kForeign);
  }
}

bool WorkerFetchContextImpl::IsControlledByServiceWorker() const {
  return is_controlled_by_service_worker_ ||
         (controller_version_id_ !=
          blink::mojom::kInvalidServiceWorkerVersionId);
}

void WorkerFetchContextImpl::SetDataSaverEnabled(bool enabled) {
  is_data_saver_enabled_ = enabled;
}

bool WorkerFetchContextImpl::IsDataSaverEnabled() const {
  return is_data_saver_enabled_;
}

void WorkerFetchContextImpl::SetIsOnSubframe(bool is_on_sub_frame) {
  is_on_sub_frame_ = is_on_sub_frame;
}

bool WorkerFetchContextImpl::IsOnSubframe() const {
  return is_on_sub_frame_;
}

blink::WebURL WorkerFetchContextImpl::SiteForCookies() const {
  return site_for_cookies_;
}

void WorkerFetchContextImpl::DidRunContentWithCertificateErrors(
    const blink::WebURL& url) {
  Send(new FrameHostMsg_DidRunContentWithCertificateErrors(parent_frame_id_,
                                                           url));
}

void WorkerFetchContextImpl::DidDisplayContentWithCertificateErrors(
    const blink::WebURL& url) {
  Send(new FrameHostMsg_DidDisplayContentWithCertificateErrors(parent_frame_id_,
                                                               url));
}

void WorkerFetchContextImpl::DidRunInsecureContent(
    const blink::WebSecurityOrigin& origin,
    const blink::WebURL& url) {
  Send(new FrameHostMsg_DidRunInsecureContent(
      parent_frame_id_, GURL(origin.ToString().Utf8()), url));
}

void WorkerFetchContextImpl::SetSubresourceFilterBuilder(
    std::unique_ptr<blink::WebDocumentSubresourceFilter::Builder>
        subresource_filter_builder) {
  subresource_filter_builder_ = std::move(subresource_filter_builder);
}

std::unique_ptr<blink::WebDocumentSubresourceFilter>
WorkerFetchContextImpl::TakeSubresourceFilter() {
  if (!subresource_filter_builder_)
    return nullptr;
  return std::move(subresource_filter_builder_)->Build();
}

void WorkerFetchContextImpl::set_service_worker_provider_id(int id) {
  service_worker_provider_id_ = id;
}

void WorkerFetchContextImpl::set_is_controlled_by_service_worker(bool flag) {
  is_controlled_by_service_worker_ = flag;
}

void WorkerFetchContextImpl::set_parent_frame_id(int id) {
  parent_frame_id_ = id;
}

void WorkerFetchContextImpl::set_site_for_cookies(
    const blink::WebURL& site_for_cookies) {
  site_for_cookies_ = site_for_cookies;
}

void WorkerFetchContextImpl::set_is_secure_context(bool flag) {
  is_secure_context_ = flag;
}

void WorkerFetchContextImpl::SetApplicationCacheHostID(int id) {
  appcache_host_id_ = id;
}

int WorkerFetchContextImpl::ApplicationCacheHostID() const {
  return appcache_host_id_;
}

void WorkerFetchContextImpl::SetControllerServiceWorker(
    int64_t controller_version_id) {
  controller_version_id_ = controller_version_id;
  MaybeInitializeSubresourceLoaderFactory();
}

bool WorkerFetchContextImpl::Send(IPC::Message* message) {
  return thread_safe_sender_->Send(message);
}

}  // namespace content
