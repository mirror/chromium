// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

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

void ForwardInterfaceRequestToService(const char* service_name,
                                      const std::string& interface_name,
                                      mojo::ScopedMessagePipeHandle handle,
                                      RenderProcessHost* host,
                                      const url::Origin& origin) {
  auto* connector = BrowserContext::GetConnectorFor(host->GetBrowserContext());
  connector->BindInterface(
      service_manager::Identity(service_name,
                                service_manager::mojom::kInheritUserID),
      interface_name, std::move(handle));
}

void ForwardToWebContentsObservers(const std::string& interface_name,
                                   mojo::ScopedMessagePipeHandle handle,
                                   RenderFrameHost* frame) {
  static_cast<RenderFrameHostImpl*>(frame)->delegate()->OnInterfaceRequest(
      frame, interface_name, &handle);
  DCHECK(!handle.is_valid()) << interface_name << " not handled by WebContents";
}

}  // namespace

// static
std::unique_ptr<WebInterfaceBroker> WebInterfaceBroker::Create() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  return std::make_unique<WebInterfaceBrokerImpl>();
}

// static
base::RepeatingCallback<void(const std::string&,
                             mojo::ScopedMessagePipeHandle,
                             RenderProcessHost*,
                             const url::Origin&)>
WebInterfaceBroker::CreateServiceRequestForwarder(const char* service_name) {
  return base::Bind(&ForwardInterfaceRequestToService, service_name);
}

WebInterfaceBrokerImpl::WebInterfaceBrokerImpl() = default;
WebInterfaceBrokerImpl::~WebInterfaceBrokerImpl() = default;

bool WebInterfaceBrokerImpl::TryBindInterface(
    WebContextType context_type,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle* interface_pipe,
    RenderProcessHost* host,
    const url::Origin& origin,
    mojo::ReportBadMessageCallback* bad_message_callback) {
  return TryBindInterfaceImpl(context_type, interface_name, interface_pipe,
                              nullptr, host, origin, bad_message_callback);
}

// Try binding an interface request |interface_pipe| for |interface_name|
// received from |frame|.
bool WebInterfaceBrokerImpl::TryBindInterface(
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle* interface_pipe,
    RenderFrameHost* frame,
    mojo::ReportBadMessageCallback* bad_message_callback) {
  return TryBindInterfaceImpl(WebContextType::kFrame, interface_name,
                              interface_pipe, frame, frame->GetProcess(),
                              frame->GetLastCommittedOrigin(),
                              bad_message_callback);
}

bool WebInterfaceBrokerImpl::TryBindInterfaceImpl(
    WebContextType context_type,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle* interface_pipe,
    RenderFrameHost* frame,
    RenderProcessHost* process,
    const url::Origin& origin,
    mojo::ReportBadMessageCallback* bad_message_callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(bad_message_callback && *bad_message_callback);
  DCHECK(interface_pipe && interface_pipe->is_valid());
  auto it = filters_.find(interface_name);
  if (it == filters_.end()) {
    return false;
  }
  auto access_result = it->second ? it->second->FilterInterface(
                                        context_type, process, frame, origin)
                                  : WebInterfaceFilter::Result::kAllow;
  switch (access_result) {
    case WebInterfaceFilter::Result::kBadMessage: {
      // Note: This currently does not kill the renderer process since the
      // InterfaceProvider connection is proxied via service manager. Once this
      // is complete and that proxying is removed, this will result in the
      // renderer making this request being killed.
      std::move(*bad_message_callback)
          .Run("Bad request for interface " + interface_name);
    }
    // Fall-through
    case WebInterfaceFilter::Result::kDeny: {
      DVLOG(1) << interface_name << " blocked";
      interface_pipe->reset();
      break;
    }
    case WebInterfaceFilter::Result::kAllow: {
      bool bound = (frame && frame_binder_registry_.TryBindInterface(
                                 interface_name, interface_pipe, frame)) ||
                   binder_registry_.TryBindInterface(
                       interface_name, interface_pipe, process, origin);
      DCHECK(bound);
      DCHECK(!interface_pipe->is_valid());
    }
  }
  return true;
}

void WebInterfaceBrokerImpl::AddInterfaceImpl(
    const std::string& interface_name,
    std::unique_ptr<WebInterfaceFilter> filter,
    base::RepeatingCallback<void(const std::string&,
                                 mojo::ScopedMessagePipeHandle,
                                 RenderFrameHost*)> frame_callback,
    base::RepeatingCallback<void(const std::string&,
                                 mojo::ScopedMessagePipeHandle,
                                 RenderProcessHost*,
                                 const url::Origin&)> callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  bool inserted = filters_.emplace(interface_name, std::move(filter)).second;
  DCHECK(inserted) << interface_name;
  if (frame_callback) {
    frame_binder_registry_.AddInterface(interface_name,
                                        std::move(frame_callback));
  }
  if (callback) {
    binder_registry_.AddInterface(interface_name, std::move(callback));
  }
}

void WebInterfaceBrokerImpl::AddWebContentsObserverInterface(
    const std::string& interface_name,
    std::unique_ptr<WebInterfaceFilter> filter) {
  AddInterfaceImpl(interface_name, std::move(filter),
                   base::Bind(&ForwardToWebContentsObservers), {});
}

}  // namespace content
