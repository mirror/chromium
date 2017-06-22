// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "headless/lib/browser/headless_tab_socket_impl.h"

#include "content/public/browser/devtools_agent_host.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/interface_provider.h"

namespace headless {

HeadlessTabSocketImpl::HeadlessTabSocketImpl() : listener_(nullptr) {}

HeadlessTabSocketImpl::~HeadlessTabSocketImpl() {}

void HeadlessTabSocketImpl::MaybeInstallTabSocketBindings(
    std::string target_devtools_frame_id,
    int v8_execution_context_id,
    base::Callback<void(bool)> callback,
    content::RenderFrameHost* render_frame_host) {
  std::string devtools_frame_id =
      content::DevToolsAgentHost::GetUntrustedDevToolsFrameIdForFrameTreeNodeId(
          render_frame_host->GetProcess()->GetID(),
          render_frame_host->GetFrameTreeNodeId());

  if (devtools_frame_id.empty()) {
    LOG(WARNING) << "GetUntrustedDevToolsFrameIdForFrameTreeNodeId failed. "
                 << "Did you forget to call Page.Enable?";
  }

  // Bail out if this isn't the right RenderFrameHost.
  if (devtools_frame_id != target_devtools_frame_id)
    return;

  HeadlessRenderFrameControllerPtr& frame_controller =
      render_frame_controllers_[devtools_frame_id];
  if (!frame_controller.is_bound())
    render_frame_host->GetRemoteInterfaces()->GetInterface(&frame_controller);
  frame_controller->InstallTabSocket(v8_execution_context_id, callback);

  v8_execution_context_id_to_frame_id_[v8_execution_context_id] =
      target_devtools_frame_id;
}

void HeadlessTabSocketImpl::SendMessageToContext(
    const std::string& message,
    int32_t v8_execution_context_id) {
  auto frame_id =
      v8_execution_context_id_to_frame_id_.find(v8_execution_context_id);
  if (frame_id == v8_execution_context_id_to_frame_id_.end()) {
    LOG(WARNING) << "Unknown v8_execution_context_id "
                 << v8_execution_context_id;
    return;
  }

  auto render_frame_controller =
      render_frame_controllers_.find(frame_id->second);
  if (render_frame_controller == render_frame_controllers_.end()) {
    LOG(WARNING) << "Unknown devtools_frame_id " << frame_id->second;
    return;
  }
  render_frame_controller->second->SendMessageToTabSocket(
      message, v8_execution_context_id);
}

void HeadlessTabSocketImpl::SetListener(Listener* listener) {
  MessageQueue messages;

  {
    base::AutoLock lock(lock_);
    listener_ = listener;
    if (!listener)
      return;

    std::swap(messages, from_tab_message_queue_);
  }

  for (const Message& message : messages) {
    listener_->OnMessageFromContext(message.first, message.second);
  }
}

void HeadlessTabSocketImpl::SendMessageToEmbedder(
    const std::string& message,
    int32_t v8_execution_context_id) {
  Listener* listener = nullptr;
  {
    base::AutoLock lock(lock_);
    if (listener_) {
      listener = listener_;
    } else {
      from_tab_message_queue_.emplace_back(message, v8_execution_context_id);
      return;
    }
  }

  listener->OnMessageFromContext(message, v8_execution_context_id);
}

void HeadlessTabSocketImpl::CreateMojoService(
    mojo::InterfaceRequest<TabSocket> request) {
  mojo_bindings_.AddBinding(this, std::move(request));
}

}  // namespace headless
