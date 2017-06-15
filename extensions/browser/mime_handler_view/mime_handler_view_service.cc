// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/mime_handler_view/mime_handler_view_service.h"

#include "base/lazy_instance.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"
#include "extensions/browser/api/mime_handler_private/mime_handler_private.h"
#include "extensions/browser/extension_util.h"
#include "extensions/browser/guest_view/mime_handler_view/mime_handler_stream_manager.h"
#include "extensions/browser/guest_view/mime_handler_view/mime_handler_view_guest.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/service_manager/public/cpp/bind_source_info.h"
#include "services/service_manager/public/cpp/binder_registry.h"

namespace extensions {
namespace {

static int kFirstRequestId = 1;

base::LazyInstance<
    std::unordered_map<content::RenderFrameHost*, MimeHandlerViewService*>>::
    DestructorAtExit g_render_frame_map = LAZY_INSTANCE_INITIALIZER;

content::RenderFrameHost* GetFrameFromID(int node_id) {
  if (auto* contents = content::WebContents::FromFrameTreeNodeId(node_id)) {
    return contents->UnsafeFindFrameByFrameTreeNodeId(node_id);
  }
  return nullptr;
}

}  // namespace

// static
void MimeHandlerViewService::CreateForFrame(
    content::RenderFrameHost* render_frame_host,
    const service_manager::BindSourceInfo& source_info,
    mime_handler_view::MimeHandlerViewServiceRequest request) {
  mojo::MakeStrongBinding(
      base::MakeUnique<MimeHandlerViewService>(render_frame_host),
      std::move(request));
}

// static
MimeHandlerViewService* MimeHandlerViewService::FromRenderFrameHost(
    content::RenderFrameHost* render_frame_host) {
  return g_render_frame_map.Get().at(render_frame_host);
}

MimeHandlerViewService::MimeHandlerViewService(
    content::RenderFrameHost* render_frame_host)
    : content::WebContentsObserver(
          content::WebContents::FromRenderFrameHost(render_frame_host)),
      stream_manager_(
          MimeHandlerStreamManager::Get(web_contents()->GetBrowserContext())),
      render_frame_host_(render_frame_host),
      weak_factory_(this) {
  g_render_frame_map.Get().insert({render_frame_host, this});
  stream_manager_->AddObserver(this);
}

MimeHandlerViewService::~MimeHandlerViewService() {
  g_render_frame_map.Get().erase(render_frame_host_);
  stream_manager_->RemoveObserver(this);
}

void MimeHandlerViewService::RegisterMimeHandlerViewFrame(
    int32_t target_frame_routing_id,
    const std::string& resource_url,
    bool in_plugin_document,
    RegisterMimeHandlerViewFrameCallback callback) {
  int process_id = render_frame_host_->GetProcess()->GetID();
  content::RenderFrameHost* navigating_frame =
      content::RenderFrameHost::FromID(process_id, target_frame_routing_id);
  if (!navigating_frame) {
    std::move(callback).Run(false);
    return;
  }

  int frame_tree_node_id = navigating_frame->GetFrameTreeNodeId();

  // In case of PluginDocument (i.e., tab or <iframe> navigation to a PDF), a
  // resource request for the PDF has been made. For <embed>-ed PDF we need to
  // make the request from the renderer side.
  bool rendeer_should_request_resource =
      !in_plugin_document || did_receive_stream_for_plugin_document_;

  // Remember we have already released the stream for plugin document so that
  // any future <embed> elements will make the request for resource separately.
  did_receive_stream_for_plugin_document_ =
      did_receive_stream_for_plugin_document_ ||
      !rendeer_should_request_resource;

  int stream_frame_id = rendeer_should_request_resource
                            ? target_frame_routing_id
                            : render_frame_host_->GetRoutingID();
  frames_[frame_tree_node_id].stream_frame_routing_id = stream_frame_id;
  frames_[frame_tree_node_id].resource_url = resource_url;
  std::move(callback).Run(rendeer_should_request_resource);
}

bool MimeHandlerViewService::MaybeDeferNavigation(
    content::NavigationHandle* handle,
    content::NavigationThrottle::ThrottleCheckResult* result) {
  int node_id = handle->GetFrameTreeNodeId();
  if (!frames_.count(node_id))
    return false;

  bool already_navigating =
      frames_[node_id].stream &&
      (frames_[node_id].navigation_handle || frames_[node_id].loaded);

  if (already_navigating) {
    // TODO(ekaramad): This does not allow re-navigating the frame. We should
    // at least support the cases where <embed>.src is changed.
    *result = content::NavigationThrottle::BLOCK_REQUEST;
    return true;
  }

  if (frames_[node_id].stream)
    return false;

  // TODO(ekaramad): Are there any security checks required?
  frames_[node_id].navigation_handle = handle;
  *result = content::NavigationThrottle::DEFER;
  return true;
}

void MimeHandlerViewService::RenderFrameHostChanged(
    content::RenderFrameHost* old_host,
    content::RenderFrameHost* new_host) {
  int node_id = new_host->GetFrameTreeNodeId();
  if (!frames_.count(node_id))
    return;

  // TODO(ekaramad): Can stream be actually nullptr?
  base::WeakPtr<StreamContainer> stream =
      frames_[node_id].stream ? frames_[node_id].stream->GetWeakPtr() : nullptr;

  // Provide the MimeHandler API.
  new_host->GetInterfaceRegistry()->AddInterface(
      base::Bind(&MimeHandlerServiceImpl::Create, stream));
}

void MimeHandlerViewService::RenderFrameDeleted(
    content::RenderFrameHost* render_frame_host) {
  int node_id = render_frame_host->GetFrameTreeNodeId();
  if (!frames_.count(node_id))
    return;

  if (!extensions::util::ForPDFExtension(
          render_frame_host->GetSiteInstance()->GetSiteURL())) {
    return;
  }
  frames_.erase(node_id);
}

void MimeHandlerViewService::DidFinishNavigation(
    content::NavigationHandle* handle) {
  int node_id = handle->GetFrameTreeNodeId();
  if (!frames_.count(node_id))
    return;

  DCHECK_EQ(frames_[node_id].navigation_handle, handle);
  frames_[node_id].loaded = true;
  frames_[node_id].navigation_handle = nullptr;
}

void MimeHandlerViewService::OnStreamAdded(int process_id,
                                           int routing_id,
                                           const std::string& view_id) {
  if (process_id != render_frame_host_->GetProcess()->GetID())
    return;

  // We will take ownership of the stream regardless of what might have
  // happended to the navigating frmae.
  std::unique_ptr<StreamContainer> stream =
      stream_manager_->ReleaseStream(view_id);

  int node_id = -1;
  for (auto& pair : frames_) {
    if (pair.second.stream_frame_routing_id == routing_id) {
      node_id = pair.first;
      break;
    }
  }

  if (node_id == -1) {
    // Frame was detached/deleted before we received the stream.
    return;
  }

  frames_[node_id].stream = std::move(stream);
  if (frames_[node_id].navigation_handle) {
    // The navigation was paused because we were waiting on the stream.
    frames_[node_id].navigation_handle->Resume();
  }
}

}  // namespace extensions
