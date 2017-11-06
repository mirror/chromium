// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/mime_handler_view/mime_handler_view_manager_host.h"

#include "base/feature_list.h"
#include "base/guid.h"
#include "base/lazy_instance.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/child_process_security_policy.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/stream_info.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_features.h"
#include "extensions/browser/api/mime_handler_private/mime_handler_private.h"
#include "extensions/browser/bad_message.h"
#include "extensions/browser/extension_util.h"
#include "extensions/browser/guest_view/mime_handler_view/mime_handler_view_guest.h"
#include "extensions/common/api/mime_handler.mojom.h"
#include "extensions/common/constants.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "ui/base/page_transition_types.h"

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

  if (!util::IsPdfExtensionUrl(
          render_frame_host->GetSiteInstance()->GetSiteURL())) {
    return;
  }

  GetRendererInterface()->FrameRemoved(
      controller_map_[frame_tree_node_id].stream_id);
  controller_map_.erase(frame_tree_node_id);
  registry_map_.erase(frame_tree_node_id);

  // TODO(ekaramad): When all frames are deleted there seems to be no reason to
  // keep |this| around. Maybe try deleting it in a graceful manner?
  // ditto for the renderer side.
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

  if (controller_map_[frame_tree_node_id].stream_container) {
    // Providing the destination frame with the required MimeHandlerService API.
    // If the registry contains the service already, remove it first.
    if (registry_map_.count(frame_tree_node_id)) {
      registry_map_[frame_tree_node_id].RemoveInterface(
          MimeHandlerServiceImpl::Name_);
    }
    registry_map_[frame_tree_node_id].AddInterface(base::Bind(
        &MimeHandlerServiceImpl::Create,
        controller_map_[frame_tree_node_id].stream_container->GetWeakPtr()));
  }
}

void MimeHandlerViewManagerHost::DocumentLoadedInFrame(
    content::RenderFrameHost* render_frame_host) {
  int frame_tree_node_id = render_frame_host->GetFrameTreeNodeId();
  if (!controller_map_.count(frame_tree_node_id))
    return;

  DCHECK(util::IsPdfExtensionUrl(
      render_frame_host->GetSiteInstance()->GetSiteURL()));

  GetRendererInterface()->DocumentLoaded(
      controller_map_[frame_tree_node_id].stream_id);
}

void MimeHandlerViewManagerHost::OnInterfaceRequestFromFrame(
    content::RenderFrameHost* render_frame_host,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle* interface_pipe) {
  if (interface_name != MimeHandlerServiceImpl::Name_)
    return;

  if (!util::IsPdfExtensionUrl(
          render_frame_host->GetSiteInstance()->GetSiteURL())) {
    // MimeHandlerServiceImpl should be only applied to frames loading the PDF
    // extension.
    return;
  }

  registry_map_[render_frame_host->GetFrameTreeNodeId()].TryBindInterface(
      interface_name, interface_pipe);
}

void MimeHandlerViewManagerHost::ReleaseStream(int32_t routing_id,
                                               const std::string& stream_id) {
  if (!base::IsValidGUID(stream_id)) {
    // This is not a valid GUID. Renderer is misbehaving.
    bad_message::ReceivedBadMessage(render_frame_host_->GetProcess(),
                                    bad_message::MHVM_INVALID_STREAM_ID);
    return;
  }

  int32_t frame_tree_node_id =
      content::RenderFrameHost::GetFrameTreeNodeIdForRoutingId(
          render_frame_host_->GetProcess()->GetID(), routing_id);

  if (frame_tree_node_id == content::RenderFrameHost::kNoFrameTreeNodeId) {
    // The routing ID did not match any proxy or frame. Renderer is misbehaving.
    bad_message::ReceivedBadMessage(render_frame_host_->GetProcess(),
                                    bad_message::MHVM_INVALID_ROUTING_ID);
    return;
  }

  controller_map_[frame_tree_node_id].stream_id = stream_id;
  controller_map_[frame_tree_node_id].stream_container =
      GetStreamManager()->ReleaseStream(stream_id);

  GURL extension_url(base::StringPrintf(
      "%s://%s/index.html?%s", kExtensionScheme,
      extension_misc::kPdfExtensionId,
      controller_map_[frame_tree_node_id].GetResourceURLString().c_str()));

  content::NavigationController::LoadURLParams params(extension_url);
  params.frame_tree_node_id = frame_tree_node_id;
  params.transition_type = ui::PAGE_TRANSITION_MANUAL_SUBFRAME;

  // TODO(ekaramad): Verify if we need a more detailed load URL
  web_contents()->GetController().LoadURLWithParams(params);
}

void MimeHandlerViewManagerHost::DropStream(const std::string& stream_id) {
  int32_t frame_tree_node_id = content::RenderFrameHost::kNoFrameTreeNodeId;
  for (const auto& pair : controller_map_) {
    if (pair.second.stream_id == stream_id) {
      frame_tree_node_id = pair.first;
      break;
    }
  }

  if (frame_tree_node_id == content::RenderFrameHost::kNoFrameTreeNodeId)
    return;

  registry_map_.erase(frame_tree_node_id);
  controller_map_[frame_tree_node_id].stream_id.clear();
  controller_map_[frame_tree_node_id].stream_container->Abort(
      base::Bind(&MimeHandlerViewManagerHost::OnStreamAborted,
                 weak_factory_.GetWeakPtr(), frame_tree_node_id, stream_id));
}

void MimeHandlerViewManagerHost::OnStreamAborted(int32_t frame_tree_node_id,
                                                 const std::string& stream_id) {
  controller_map_[frame_tree_node_id].stream_container.reset(nullptr);
  GetRendererInterface()->DidAbortStream(stream_id);
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

std::string MimeHandlerViewManagerHost::ControllerInfo::GetResourceURLString()
    const {
  if (!stream_container)
    return "";
  return stream_container->stream_info()->original_url.spec();
}

}  // namespace extensions
