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
#include "extensions/browser/mime_handler_view/mime_handler_view_extension_navigation_throttle.h"
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
    content::RenderFrameHost* rfh,
    mime_handler_view::MimeHandlerViewManagerHostRequest request) {
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
  registry_for_manager_host_.AddInterface(base::Bind(
      &MimeHandlerViewManagerHost::CreateInterface, render_frame_host));
}

MimeHandlerViewManagerHost::~MimeHandlerViewManagerHost() {
  g_render_frame_map.Get().erase(render_frame_host_);
  for (auto& pair : navigation_helpers_) {
    if (pair.second.stream_container)
      pair.second.stream_container->Abort(base::Closure());
  }
}

void MimeHandlerViewManagerHost::RenderFrameCreated(
    content::RenderFrameHost* render_frame_host) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (render_frame_host->GetParent() != render_frame_host_)
    return;

  if (!util::IsPdfExtensionUrl(
          render_frame_host->GetSiteInstance()->GetSiteURL())) {
    return;
  }

  int32_t frame_tree_node_id = render_frame_host->GetFrameTreeNodeId();
  navigation_helpers_[frame_tree_node_id].destination_render_frame_host =
      render_frame_host;
  if (navigation_helpers_[frame_tree_node_id].stream_container)
    AddMimeHandlerServiceInterface(frame_tree_node_id);
}

void MimeHandlerViewManagerHost::RenderFrameDeleted(
    content::RenderFrameHost* render_frame_host) {
  if (render_frame_host_ == render_frame_host) {
    delete this;
    return;
  }

  int frame_tree_node_id = render_frame_host->GetFrameTreeNodeId();
  if (!navigation_helpers_.count(frame_tree_node_id))
    return;

  if (navigation_helpers_[frame_tree_node_id].state >= kTargetFrameLoading &&
      !util::IsPdfExtensionUrl(
          render_frame_host->GetSiteInstance()->GetSiteURL())) {
    // This is expected during navigation, i.e., the content frame swaps out
    // and therefore the RenderFrameHost is deleted.
    return;
  }

  RemoveFrameState(frame_tree_node_id);
}

void MimeHandlerViewManagerHost::DidFinishNavigation(
    content::NavigationHandle* handle) {
  int frame_tree_node_id = handle->GetFrameTreeNodeId();
  if (!navigation_helpers_.count(frame_tree_node_id))
    return;

  // Lose the navigation handle (which will be deleted soon).
  navigation_helpers_[frame_tree_node_id].navigation_throttle = nullptr;
}

void MimeHandlerViewManagerHost::DocumentLoadedInFrame(
    content::RenderFrameHost* render_frame_host) {
  int frame_tree_node_id = render_frame_host->GetFrameTreeNodeId();
  if (!navigation_helpers_.count(frame_tree_node_id))
    return;

  DCHECK_EQ(render_frame_host->GetParent(), render_frame_host_);
  DCHECK(!navigation_helpers_[frame_tree_node_id].navigation_throttle);
  DCHECK(util::IsPdfExtensionUrl(
      render_frame_host->GetSiteInstance()->GetSiteURL()));
  DCHECK(GetRendererInterface());
  GetRendererInterface()->DocumentLoaded(frame_tree_node_id);
  navigation_helpers_[frame_tree_node_id].state = kDidLoadDocument;
}

void MimeHandlerViewManagerHost::OnInterfaceRequestFromFrame(
    content::RenderFrameHost* render_frame_host,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle* interface_pipe) {
  if (render_frame_host_ == render_frame_host) {
    registry_for_manager_host_.TryBindInterface(interface_name, interface_pipe);
    return;
  }

  if (util::IsPdfExtensionUrl(
          render_frame_host->GetSiteInstance()->GetSiteURL())) {
    registry_map_[render_frame_host->GetFrameTreeNodeId()].TryBindInterface(
        interface_name, interface_pipe);
  }
}

void MimeHandlerViewManagerHost::ReleaseStream(int32_t frame_tree_node_id,
                                               const std::string& view_id) {
  if (!navigation_helpers_.count(frame_tree_node_id))
    return;

  navigation_helpers_[frame_tree_node_id].stream_container =
      GetStreamManager()->ReleaseStream(view_id);
  if (navigation_helpers_[frame_tree_node_id].stream_container) {
    if (navigation_helpers_[frame_tree_node_id].destination_render_frame_host)
      AddMimeHandlerServiceInterface(frame_tree_node_id);
    navigation_helpers_[frame_tree_node_id].navigation_throttle->DoResume();
    navigation_helpers_[frame_tree_node_id].state = kDidResumeNavigation;
  } else {
    // TODO(ekaramad): Alternatively, we could proceed with navigation and show
    // the user an empty PDF extension with JS errors thrown in console due to
    // missing stream information.
    navigation_helpers_[frame_tree_node_id]
        .navigation_throttle->DoCancelDeferredNavigation(
            content::NavigationThrottle::CANCEL_AND_IGNORE);
  }

  navigation_helpers_[frame_tree_node_id].navigation_throttle = nullptr;
}

bool MimeHandlerViewManagerHost::InterceptNavigation(
    MimeHandlerViewExtensionNavigationThrottle* throttle) {
  if (!GetRendererInterface() || !web_contents())
    return false;

  content::NavigationHandle* handle = throttle->navigation_handle();
  int32_t frame_tree_node_id = handle->GetFrameTreeNodeId();
  bool maybe_cross_process_navigation =
      !util::IsPdfExtensionUrl(handle->GetStartingSiteInstance()->GetSiteURL());

  DCHECK(
      maybe_cross_process_navigation ||
      (navigation_helpers_.count(frame_tree_node_id) &&
       navigation_helpers_[frame_tree_node_id].state >= kDidPauseNavigation));

  navigation_helpers_[frame_tree_node_id].navigation_throttle = throttle;
  navigation_helpers_[frame_tree_node_id].resource_url =
      handle->GetURL().query();
  navigation_helpers_[frame_tree_node_id].state = kDidPauseNavigation;

  content::RenderFrameHost* content_frame =
      web_contents()->FindFrameByFrameTreeNodeId(
          frame_tree_node_id, render_frame_host_->GetProcess()->GetID());

  DCHECK_EQ(!!content_frame, maybe_cross_process_navigation);
  int32_t routing_id = maybe_cross_process_navigation
                           ? content_frame->GetRoutingID()
                           : MSG_ROUTING_NONE;
  GetRendererInterface()->DidPauseNavigation(routing_id, frame_tree_node_id);
  if (!maybe_cross_process_navigation)
    RemoveMimeHandlerServiceInterface(frame_tree_node_id);
  return true;
}

void MimeHandlerViewManagerHost::OnNavigationThrottleDestroyed(
    MimeHandlerViewExtensionNavigationThrottle* throttle) {
  int frame_tree_node_id = throttle->navigation_handle()->GetFrameTreeNodeId();
  if (navigation_helpers_.count(frame_tree_node_id))
    navigation_helpers_[frame_tree_node_id].navigation_throttle = nullptr;
}

void MimeHandlerViewManagerHost::AddMimeHandlerServiceInterface(
    int32_t frame_tree_node_id) {
  registry_map_[frame_tree_node_id].AddInterface(base::Bind(
      &MimeHandlerServiceImpl::Create,
      navigation_helpers_[frame_tree_node_id].stream_container->GetWeakPtr()));
  navigation_helpers_[frame_tree_node_id].state = kTargetFrameLoading;
}

void MimeHandlerViewManagerHost::RemoveMimeHandlerServiceInterface(
    int32_t frame_tree_node_id) {
  registry_map_[frame_tree_node_id].RemoveInterface<MimeHandlerServiceImpl>();
}

void MimeHandlerViewManagerHost::RemoveFrameState(int32_t frame_tree_node_id) {
  if (navigation_helpers_[frame_tree_node_id].state >= kDidPauseNavigation)
    GetRendererInterface()->FrameRemoved(frame_tree_node_id);

  navigation_helpers_.erase(frame_tree_node_id);
  registry_map_.erase(frame_tree_node_id);
  if (!navigation_helpers_.size())
    delete this;
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

MimeHandlerViewManagerHost::NavigationHelper::NavigationHelper() {}
MimeHandlerViewManagerHost::NavigationHelper::~NavigationHelper() {}

}  // namespace extensions
