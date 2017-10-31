// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/mime_handler_view/mime_handler_view_manager_host.h"

#include "base/feature_list.h"
#include "base/lazy_instance.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/child_process_security_policy.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_features.h"
#include "extensions/browser/api/mime_handler_private/mime_handler_private.h"
#include "extensions/browser/extension_util.h"
#include "extensions/browser/guest_view/mime_handler_view/mime_handler_view_guest.h"
#include "extensions/common/api/mime_handler.mojom.h"
#include "extensions/common/constants.h"
#include "services/service_manager/public/cpp/interface_provider.h"

namespace extensions {
namespace {
base::LazyInstance<std::unordered_map<content::RenderFrameHost*,
                                      MimeHandlerViewManagerHost*>>::
    DestructorAtExit g_render_frame_map = LAZY_INSTANCE_INITIALIZER;
}  // namespace

// static.
void MimeHandlerViewManagerHost::CreateInterface(
    mime_handler_view::MimeHandlerViewManagerHostRequest request,
    content::RenderFrameHost* rfh) {
  if (auto* host = Get(rfh))
    host->binding_.Bind(std::move(request));
}

// static
MimeHandlerViewManagerHost* MimeHandlerViewManagerHost::Get(
    content::RenderFrameHost* render_frame_host) {
  return g_render_frame_map.Get().count(render_frame_host)
             ? g_render_frame_map.Get().at(render_frame_host)
             : new MimeHandlerViewManagerHost(render_frame_host);
}

MimeHandlerViewManagerHost::MimeHandlerViewManagerHost(
    content::RenderFrameHost* render_frame_host)
    : content::WebContentsObserver(
          content::WebContents::FromRenderFrameHost(render_frame_host)),
      render_frame_host_(render_frame_host),
      binding_(this),
      weak_factory_(this) {
  g_render_frame_map.Get().insert({render_frame_host, this});
}

MimeHandlerViewManagerHost::~MimeHandlerViewManagerHost() {
  g_render_frame_map.Get().erase(render_frame_host_);
  registry_map_.clear();
  if (binding_.is_bound())
    binding_.Close();
}

void MimeHandlerViewManagerHost::RenderFrameDeleted(
    content::RenderFrameHost* render_frame_host) {
  if (render_frame_host_ == render_frame_host) {
    delete this;
    return;
  }

  int frame_tree_node_id = render_frame_host->GetFrameTreeNodeId();
  if (!controller_map_.count(frame_tree_node_id))
    return;

  if (render_frame_host_->GetSiteInstance() ==
      render_frame_host->GetSiteInstance()) {
    // This is expected during navigation, i.e., the content frame swaps out
    // and therefore the RenderFrameHost is deleted.
    return;
  }
  RemoveFrameState(frame_tree_node_id);
}

void MimeHandlerViewManagerHost::ReadyToCommitNavigation(
    content::NavigationHandle* handle) {
  int frame_tree_node_id = handle->GetFrameTreeNodeId();
  if (frame_tree_node_id == render_frame_host_->GetFrameTreeNodeId())
    delete this;
}

void MimeHandlerViewManagerHost::DidFinishNavigation(
    content::NavigationHandle* handle) {
  int frame_tree_node_id = handle->GetFrameTreeNodeId();
  if (!controller_map_.count(frame_tree_node_id))
    return;

  controller_map_[frame_tree_node_id].destination_render_frame_host =
      handle->GetRenderFrameHost();
  AddMimeHandlerServiceInterface(frame_tree_node_id);
}

void MimeHandlerViewManagerHost::DocumentLoadedInFrame(
    content::RenderFrameHost* render_frame_host) {
  int frame_tree_node_id = render_frame_host->GetFrameTreeNodeId();
  if (!controller_map_.count(frame_tree_node_id))
    return;

  int original_render_frame_routing_id =
      controller_map_[frame_tree_node_id].original_render_frame_routing_id;
  DCHECK_EQ(render_frame_host->GetParent(), render_frame_host_);
  DCHECK(GetRendererInterface());
  if (!util::IsPdfExtensionUrl(
          render_frame_host->GetSiteInstance()->GetSiteURL())) {
    GetRendererInterface()->FrameRemoved(original_render_frame_routing_id);
    return;
  }

  GetRendererInterface()->DocumentLoaded(original_render_frame_routing_id);
}

void MimeHandlerViewManagerHost::OnInterfaceRequestFromFrame(
    content::RenderFrameHost* render_frame_host,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle* interface_pipe) {
  if (util::IsPdfExtensionUrl(
          render_frame_host->GetSiteInstance()->GetSiteURL())) {
    registry_map_[render_frame_host->GetFrameTreeNodeId()].TryBindInterface(
        interface_name, interface_pipe);
  }
}

void MimeHandlerViewManagerHost::ReleaseStream(
    int32_t original_render_frame_routing_id,
    const std::string& view_id) {
  content::RenderFrameHost* render_frame_host =
      content::RenderFrameHost::FromID(
          render_frame_host_->GetProcess()->GetID(),
          original_render_frame_routing_id);
  if (!render_frame_host)
    GetRendererInterface()->FrameRemoved(original_render_frame_routing_id);

  int32_t frame_tree_node_id = render_frame_host->GetFrameTreeNodeId();
  controller_map_[frame_tree_node_id].original_render_frame_routing_id =
      original_render_frame_routing_id;

  controller_map_[frame_tree_node_id].stream_container =
      GetStreamManager()->ReleaseStream(view_id);

  // Notify the renderer so that it can start loading the extension. The
  // required interface for MimeHandlerService will be added to the destination
  // frame.
  GetRendererInterface()->OnStreamReleased(original_render_frame_routing_id);
}

void MimeHandlerViewManagerHost::AddMimeHandlerServiceInterface(
    int32_t frame_tree_node_id) {
  if (!controller_map_[frame_tree_node_id].stream_container)
    return;

  registry_map_[frame_tree_node_id].AddInterface(base::Bind(
      &MimeHandlerServiceImpl::Create,
      controller_map_[frame_tree_node_id].stream_container->GetWeakPtr()));
}

void MimeHandlerViewManagerHost::RemoveFrameState(int32_t frame_tree_node_id) {
  GetRendererInterface()->FrameRemoved(frame_tree_node_id);

  controller_map_.erase(frame_tree_node_id);
  registry_map_.erase(frame_tree_node_id);
  // TODO(ekaramad): We could delete |this| when there are no frame controllers
  // left.
}

mime_handler_view::MimeHandlerViewManager*
MimeHandlerViewManagerHost::GetRendererInterface() {
  if (!renderer_interface_.get()) {
    render_frame_host_->GetRemoteInterfaces()->GetInterface(
        &renderer_interface_);
  }
  return renderer_interface_.get();
}

MimeHandlerStreamManager* MimeHandlerViewManagerHost::GetStreamManager() {
  return MimeHandlerStreamManager::Get(web_contents()->GetBrowserContext());
}

MimeHandlerViewManagerHost::ControllerInfo::ControllerInfo() {}

MimeHandlerViewManagerHost::ControllerInfo::~ControllerInfo() {}

}  // namespace extensions
