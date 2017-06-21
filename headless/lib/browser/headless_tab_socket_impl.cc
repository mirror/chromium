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
    int world_id,
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
  frame_controller->InstallTabSocket(devtools_frame_id, world_id, callback);
}

void HeadlessTabSocketImpl::SendMessageToTab(
    const std::string& message,
    const std::string& devtools_frame_id,
    int32_t world_id) {
  auto find_it = render_frame_controllers_.find(devtools_frame_id);
  if (find_it == render_frame_controllers_.end()) {
    LOG(WARNING) << "Unknown devtools_frame_id " << devtools_frame_id;
    return;
  }
  find_it->second->SendMessageToTabSocket(message, world_id);
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
    listener_->OnMessageFromTab(std::get<0>(message), std::get<1>(message),
                                std::get<2>(message));
  }
}

void HeadlessTabSocketImpl::SendMessageToEmbedder(
    const std::string& message,
    const std::string& devtools_frame_id,
    int32_t world_id) {
  Listener* listener = nullptr;
  {
    base::AutoLock lock(lock_);
    if (listener_) {
      listener = listener_;
    } else {
      from_tab_message_queue_.emplace_back(message, devtools_frame_id,
                                           world_id);
      return;
    }
  }

  listener->OnMessageFromTab(message, devtools_frame_id, world_id);
}

void HeadlessTabSocketImpl::CreateMojoService(
    mojo::InterfaceRequest<TabSocket> request) {
  mojo_bindings_.AddBinding(this, std::move(request));
}

}  // namespace headless
