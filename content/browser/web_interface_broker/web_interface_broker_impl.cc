// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/web_interface_broker/web_interface_broker_impl.h"

#include <memory>
#include <utility>

#include "base/macros.h"
#include "content/browser/web_interface_broker/binder_holder.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_context_type.h"
#include "content/public/browser/web_interface_filter.h"

namespace content {

WebInterfaceBrokerImpl::WebInterfaceBrokerImpl(
    WebInterfaceBrokerBuilder::WorkerDelegate worker_delegate,
    WebInterfaceBrokerBuilder::FrameDelegate frame_delegate,
    WebInterfaceBrokerBuilder::AssociatedFrameDelegate
        associated_frame_delegate,
    std::unordered_map<std::string, BinderHolder> binders)
    : binders_(std::move(binders)),
      worker_delegate_(std::move(worker_delegate)),
      frame_delegate_(std::move(frame_delegate)),
      associated_frame_delegate_(std::move(associated_frame_delegate)) {}

WebInterfaceBrokerImpl::~WebInterfaceBrokerImpl() = default;

void WebInterfaceBrokerImpl::BindInterfaceForWorker(
    WebContextType context_type,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe,
    RenderProcessHost* host,
    const url::Origin& origin,
    mojo::ReportBadMessageCallback bad_message_callback) {
  if (auto* binder = GetBinder(interface_name)) {
    binder->BindInterfaceForWorker(context_type, std::move(interface_pipe),
                                   host, origin,
                                   std::move(bad_message_callback));
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
  if (auto* binder = GetBinder(interface_name)) {
    binder->BindInterfaceForFrame(std::move(interface_pipe), frame,
                                  std::move(bad_message_callback));
    return;
  }
  if (frame_delegate_) {
    frame_delegate_.Run(interface_name, std::move(interface_pipe), frame,
                        std::move(bad_message_callback));
  }
}

void WebInterfaceBrokerImpl::BindAssociatedInterfaceForFrame(
    const std::string& interface_name,
    mojo::ScopedInterfaceEndpointHandle interface_pipe,
    RenderFrameHost* frame,
    mojo::ReportBadMessageCallback bad_message_callback) {
  if (auto* binder = GetBinder(interface_name)) {
    binder->BindAssociatedInterfaceForFrame(std::move(interface_pipe), frame,
                                            std::move(bad_message_callback));
    return;
  }
  if (associated_frame_delegate_) {
    associated_frame_delegate_.Run(interface_name, std::move(interface_pipe),
                                   frame, std::move(bad_message_callback));
  }
}

const BinderHolder* WebInterfaceBrokerImpl::GetBinder(
    const std::string& interface_name) {
  auto it = binders_.find(interface_name);
  if (it == binders_.end())
    return nullptr;
  return &it->second;
}

}  // namespace content
