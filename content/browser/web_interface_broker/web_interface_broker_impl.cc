// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/web_interface_broker/web_interface_broker_impl.h"

#include <memory>
#include <utility>

#include "base/macros.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_interface_filter.h"

namespace content {

WebInterfaceBrokerImpl::WebInterfaceBrokerImpl(
    WebInterfaceBrokerBuilder::WorkerDelegate worker_delegate,
    WebInterfaceBrokerBuilder::FrameDelegate frame_delegate,
    std::unique_ptr<service_manager::BinderRegistryWithArgs<RenderProcessHost*,
                                                            const url::Origin&>>
        binder_registry,
    std::unique_ptr<service_manager::BinderRegistryWithArgs<RenderFrameHost*>>
        frame_binder_registry,
    std::unordered_map<std::string, std::unique_ptr<WebInterfaceFilter>>
        filters)
    : binder_registry_(std::move(binder_registry)),
      frame_binder_registry_(std::move(frame_binder_registry)),
      filters_(std::move(filters)),
      worker_delegate_(std::move(worker_delegate)),
      frame_delegate_(std::move(frame_delegate)) {}

WebInterfaceBrokerImpl::~WebInterfaceBrokerImpl() = default;

void WebInterfaceBrokerImpl::BindInterfaceForWorker(
    WebContextType context_type,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe,
    RenderProcessHost* host,
    const url::Origin& origin,
    mojo::ReportBadMessageCallback bad_message_callback) {
  if (BindInterfaceImpl(context_type, interface_name, &interface_pipe, nullptr,
                        host, origin, &bad_message_callback)) {
    return;
  }
  if (worker_delegate_) {
    worker_delegate_.Run(context_type, interface_name,
                         std::move(interface_pipe), host, origin,
                         std::move(bad_message_callback));
  }
}

// Try binding an interface request |interface_pipe| for |interface_name|
// received from |frame|.
void WebInterfaceBrokerImpl::BindInterfaceForFrame(
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe,
    RenderFrameHost* frame,
    mojo::ReportBadMessageCallback bad_message_callback) {
  if (BindInterfaceImpl(WebContextType::kFrame, interface_name, &interface_pipe,
                        frame, frame->GetProcess(),
                        frame->GetLastCommittedOrigin(),
                        &bad_message_callback)) {
    return;
  }
  if (frame_delegate_) {
    frame_delegate_.Run(interface_name, std::move(interface_pipe), frame,
                        std::move(bad_message_callback));
  }
}

bool WebInterfaceBrokerImpl::BindInterfaceImpl(
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
      bool bound = (frame && frame_binder_registry_->TryBindInterface(
                                 interface_name, interface_pipe, frame));
      if (!bound) {
        bound = binder_registry_->TryBindInterface(
            interface_name, interface_pipe, process, origin);
      }
      DCHECK(bound);
      DCHECK(!interface_pipe->is_valid());
    }
  }

  return true;
}

}  // namespace content
