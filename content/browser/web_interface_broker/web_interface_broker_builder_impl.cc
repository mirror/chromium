// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/web_interface_broker/web_interface_broker_builder_impl.h"

#include <memory>
#include <utility>

#include "base/macros.h"
#include "content/browser/frame_host/render_frame_host_delegate.h"
#include "content/browser/frame_host/render_frame_host_impl.h"
#include "content/browser/web_interface_broker/web_interface_broker_impl.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_interface_filter.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/service_manager/public/cpp/identity.h"

namespace content {
namespace {

void ForwardInterfaceRequestToService(base::StringPiece service_name,
                                      const std::string& interface_name,
                                      mojo::ScopedMessagePipeHandle handle,
                                      RenderProcessHost* host,
                                      const url::Origin& origin) {
  auto* connector = BrowserContext::GetConnectorFor(host->GetBrowserContext());
  DCHECK(connector);
  connector->BindInterface(
      service_manager::Identity(service_name.as_string(),
                                service_manager::mojom::kInheritUserID),
      interface_name, std::move(handle));
}

void ForwardToWebContentsObservers(const std::string& interface_name,
                                   mojo::ScopedMessagePipeHandle handle,
                                   RenderFrameHost* frame) {
  static_cast<RenderFrameHostImpl*>(frame)->delegate()->OnInterfaceRequest(
      frame, interface_name, &handle);
  DVLOG_IF(2, !handle.is_valid())
      << interface_name << " not handled by WebContents";
}

}  // namespace

// static
std::unique_ptr<WebInterfaceBrokerBuilder> WebInterfaceBrokerBuilder::Create(
    WorkerDelegate worker_delegate,
    FrameDelegate frame_delegate) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  return std::make_unique<WebInterfaceBrokerBuilderImpl>(
      std::move(worker_delegate), std::move(frame_delegate));
}

std::unique_ptr<WebInterfaceBroker> WebInterfaceBrokerBuilderImpl::Build() {
  return std::make_unique<WebInterfaceBrokerImpl>(
      std::move(worker_delegate_), std::move(frame_delegate_),
      std::move(binder_registry_), std::move(frame_binder_registry_),
      std::move(filters_));
}

WebInterfaceBrokerBuilderImpl::WebInterfaceBrokerBuilderImpl(
    WorkerDelegate worker_delegate,
    FrameDelegate frame_delegate)
    : binder_registry_(
          std::make_unique<
              service_manager::BinderRegistryWithArgs<RenderProcessHost*,
                                                      const url::Origin&>>()),
      frame_binder_registry_(
          std::make_unique<
              service_manager::BinderRegistryWithArgs<RenderFrameHost*>>()),
      worker_delegate_(std::move(worker_delegate)),
      frame_delegate_(std::move(frame_delegate)) {}

WebInterfaceBrokerBuilderImpl::~WebInterfaceBrokerBuilderImpl() = default;

void WebInterfaceBrokerBuilderImpl::AddInterfaceImpl(
    base::StringPiece interface_name,
    std::unique_ptr<WebInterfaceFilter> filter,
    base::RepeatingCallback<void(const std::string&,
                                 mojo::ScopedMessagePipeHandle,
                                 RenderFrameHost*)> frame_callback,
    base::RepeatingCallback<void(const std::string&,
                                 mojo::ScopedMessagePipeHandle,
                                 RenderProcessHost*,
                                 const url::Origin&)> callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(filter);

  const std::string interface_name_str = interface_name.as_string();
  if (frame_callback) {
    frame_binder_registry_->AddInterface(interface_name_str,
                                         std::move(frame_callback));
  }
  if (callback) {
    binder_registry_->AddInterface(interface_name_str, std::move(callback));
  } else {
    filter = CreateFilterBundle(
        CreateContextTypeInterfaceFilter({WebContextType::kFrame}),
        std::move(filter));
  }
  bool inserted =
      filters_.emplace(std::move(interface_name_str), std::move(filter)).second;
  DCHECK(inserted) << interface_name;
}

void WebInterfaceBrokerBuilderImpl::AddWebContentsObserverInterfaceImpl(
    base::StringPiece interface_name,
    std::unique_ptr<WebInterfaceFilter> filter) {
  AddInterfaceImpl(interface_name, std::move(filter),
                   base::BindRepeating(&ForwardToWebContentsObservers), {});
}

void WebInterfaceBrokerBuilderImpl::ForwardInterfaceToService(
    base::StringPiece interface_name,
    std::unique_ptr<WebInterfaceFilter> filter,
    base::StringPiece service_name) {
  AddInterfaceImpl(
      interface_name, std::move(filter), {},
      base::BindRepeating(&ForwardInterfaceRequestToService, service_name));
}

}  // namespace content
