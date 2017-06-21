// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "headless/lib/renderer/headless_render_frame_controller_impl.h"

#include "services/service_manager/public/cpp/bind_source_info.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/interface_provider.h"

namespace headless {

HeadlessRenderFrameControllerImpl::HeadlessRenderFrameControllerImpl(
    content::RenderFrame* render_frame)
    : content::RenderFrameObserver(render_frame),
      render_frame_(render_frame),
      weak_ptr_factory_(this) {
  render_frame->GetInterfaceRegistry()->AddInterface(base::Bind(
      &HeadlessRenderFrameControllerImpl::OnRenderFrameControllerRequest,
      base::Unretained(this)));
}

HeadlessRenderFrameControllerImpl::~HeadlessRenderFrameControllerImpl() {}

void HeadlessRenderFrameControllerImpl::OnRenderFrameControllerRequest(
    const service_manager::BindSourceInfo& source_info,
    headless::HeadlessRenderFrameControllerRequest request) {
  headless_render_frame_controller_bindings_.AddBinding(this,
                                                        std::move(request));
}

void HeadlessRenderFrameControllerImpl::InstallTabSocket(
    const std::string& devtools_frame_id,
    int32_t world_id,
    InstallTabSocketCallback callback) {
  DCHECK(devtools_frame_id_.empty() ||
         devtools_frame_id_ == devtools_frame_id_);
  devtools_frame_id_ = devtools_frame_id;
  auto find_it = tab_socket_bindings_.find(world_id);
  if (find_it == tab_socket_bindings_.end()) {
    std::move(callback).Run(false);
  } else {
    std::move(callback).Run(find_it->second.InitializeTabSocketBindings());
  }
}

void HeadlessRenderFrameControllerImpl::SendMessageToTabSocket(
    const std::string& message,
    int32_t world_id) {
  auto find_it = tab_socket_bindings_.find(world_id);
  if (find_it == tab_socket_bindings_.end()) {
    LOG(WARNING) << "Dropping message for " << world_id
                 << " because the world doesn't exist.";
    return;
  }

  find_it->second.OnMessageFromEmbedder(message);
}

// content::RenderFrameObserverW implementation:
void HeadlessRenderFrameControllerImpl::DidCreateScriptContext(
    v8::Local<v8::Context> context,
    int world_id) {
  auto find_it = tab_socket_bindings_.find(world_id);
  if (find_it != tab_socket_bindings_.end())
    tab_socket_bindings_.erase(find_it);

  tab_socket_bindings_.emplace(
      std::piecewise_construct, std::forward_as_tuple(world_id),
      std::forward_as_tuple(this, render_frame_, context, world_id));
}

void HeadlessRenderFrameControllerImpl::WillReleaseScriptContext(
    v8::Local<v8::Context> context,
    int world_id) {
  tab_socket_bindings_.erase(world_id);
}

void HeadlessRenderFrameControllerImpl::OnDestruct() {
  delete this;
}

headless::TabSocketPtr&
HeadlessRenderFrameControllerImpl::EnsureTabSocketPtr() {
  if (!tab_socket_ptr_.is_bound()) {
    render_frame_->GetRemoteInterfaces()->GetInterface(
        mojo::MakeRequest(&tab_socket_ptr_));
  }
  return tab_socket_ptr_;
}

}  // namespace headless
