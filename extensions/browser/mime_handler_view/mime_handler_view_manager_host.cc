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

// static
bool MimeHandlerViewManagerHost::MaybeDeferNavigation(
    content::NavigationHandle* handle,
    content::NavigationThrottle::ThrottleCheckResult* result) {
  if (!base::FeatureList::IsEnabled(features::kPdfExtensionInOutOfProcessFrame))
    return false;

  if (!util::IsPdfExtensionUrl(handle->GetURL()))
    return false;

  content::RenderFrameHost* parent_frame = handle->GetParentFrame();
  if (!parent_frame) {
    // A PDF resource is always embedded in the page. So when the <embed>'s
    // content frame is navigating, there has to be a parent node. Therefore,
    // any navigation to PDF extension from omnibox will not be tracked nor
    // stalled.
    return false;
  }

  // The MimeHandlerViewManagerHost for the parent frame will be notified about
  // the upcoming navigation. The navigation is always deferred until the
  // MHVMH resumes it.
  if (!Get(parent_frame)->InterceptNavigation(handle))
    return false;

  *result = content::NavigationThrottle::DEFER;

  // TODO(ekaramad): Implement the logic to defer the navigation and for stream
  // for PDF resource to be ready.
  return true;
}

// static.
void MimeHandlerViewManagerHost::CreateInterface(
    content::RenderFrameHost* rfh,
    const service_manager::BindSourceInfo& info,
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
  render_frame_host_->GetInterfaceRegistry()->AddInterface(base::Bind(
      MimeHandlerViewManagerHost::CreateInterface, render_frame_host));
}

MimeHandlerViewManagerHost::~MimeHandlerViewManagerHost() {
  g_render_frame_map.Get().erase(render_frame_host_);
  GetRendererInterface()->Destroy();
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

  // Is it even possible to have a stream container at this point?
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
      navigation_helpers_[frame_tree_node_id].frame_controller_id ==
          render_frame_host->GetRoutingID()) {
    // This is a legitimate case for deleting the original RenderFrameHost
    // (i.e., when the corresponding content frame of the <embed> swaps out).
    return;
  }

  RemoveFrameController(frame_tree_node_id);
}

void MimeHandlerViewManagerHost::DidStartNavigation(
    content::NavigationHandle* handle) {
  if (render_frame_host_->GetFrameTreeNodeId() == handle->GetFrameTreeNodeId())
    delete this;
}

void MimeHandlerViewManagerHost::DidFinishNavigation(
    content::NavigationHandle* handle) {
  int frame_tree_node_id = handle->GetFrameTreeNodeId();
  if (!navigation_helpers_.count(frame_tree_node_id))
    return;

  // Lose the navigation handle (which will be deleted soon).
  navigation_helpers_[frame_tree_node_id].navigation_handle = nullptr;
}

void MimeHandlerViewManagerHost::DocumentLoadedInFrame(
    content::RenderFrameHost* render_frame_host) {
  int frame_tree_node_id = render_frame_host->GetFrameTreeNodeId();
  if (!navigation_helpers_.count(frame_tree_node_id))
    return;

  DCHECK_EQ(render_frame_host->GetParent(), render_frame_host_);
  DCHECK(!navigation_helpers_[frame_tree_node_id].navigation_handle);
  DCHECK(util::IsPdfExtensionUrl(
      render_frame_host->GetSiteInstance()->GetSiteURL()));
  DCHECK(GetRendererInterface());
  GetRendererInterface()->DocumentLoaded(frame_tree_node_id);
  navigation_helpers_[frame_tree_node_id].state = kDidLoadDocument;
}

void MimeHandlerViewManagerHost::ReleaseStream(int32_t frame_controller_id,
                                               const std::string& view_id) {
  int32_t frame_tree_node_id = frame_controller_id;
  if (!navigation_helpers_.count(frame_tree_node_id))
    return;

  navigation_helpers_[frame_tree_node_id].stream_container =
      GetStreamManager()->ReleaseStream(view_id);

  if (navigation_helpers_[frame_tree_node_id].stream_container) {
    if (navigation_helpers_[frame_tree_node_id].destination_render_frame_host)
      AddMimeHandlerServiceInterface(frame_tree_node_id);

    navigation_helpers_[frame_tree_node_id].navigation_handle->Resume();
    navigation_helpers_[frame_tree_node_id].state = kDidResumeNavigation;
  } else {
    // TODO(ekaramad): Alternatively, we could proceed with navigation and show
    // the user an empty PDF extension with JS errors thrown in console due to
    // missing stream information.
    navigation_helpers_[frame_tree_node_id]
        .navigation_handle->CancelDeferredNavigation(
            content::NavigationThrottle::CANCEL_AND_IGNORE);
  }
}

bool MimeHandlerViewManagerHost::InterceptNavigation(
    content::NavigationHandle* handle) {
  if (!GetRendererInterface() || !web_contents())
    return false;

  int32_t frame_tree_node_id = handle->GetFrameTreeNodeId();
  navigation_helpers_[frame_tree_node_id].frame_controller_id =
      frame_tree_node_id;
  navigation_helpers_[frame_tree_node_id].navigation_handle = handle;
  navigation_helpers_[frame_tree_node_id].resource_url =
      handle->GetURL().query();

  int routing_id_in_parent_process = MSG_ROUTING_NONE;
  if (handle->GetStartingSiteInstance() ==
      render_frame_host_->GetSiteInstance()) {
    // Used to identify the MimeHandlerFrameCotroller when the navigating frame
    // is content frame of a plugin element. In such cases, the
    // MimeHandlerFrameController might have been already created.
    content::RenderFrameHost* content_frame =
        web_contents()->FindFrameByFrameTreeNodeId(
            frame_tree_node_id, render_frame_host_->GetProcess()->GetID());
    routing_id_in_parent_process = content_frame->GetRoutingID();
  }

  GetRendererInterface()->RequestResource(
      frame_tree_node_id, routing_id_in_parent_process,
      navigation_helpers_[frame_tree_node_id].resource_url);
  navigation_helpers_[frame_tree_node_id].state = kDidRequestResource;
  return true;
}

void MimeHandlerViewManagerHost::AddMimeHandlerServiceInterface(
    int32_t frame_tree_node_id) {
  auto* binder_registry =
      navigation_helpers_[frame_tree_node_id]
          .destination_render_frame_host->GetInterfaceRegistry();
  binder_registry->RemoveInterface<mime_handler::MimeHandlerService>();
  binder_registry->AddInterface(base::Bind(
      &MimeHandlerServiceImpl::Create,
      navigation_helpers_[frame_tree_node_id].stream_container->GetWeakPtr()));
  navigation_helpers_[frame_tree_node_id].state = kTargetFrameLoading;
}

void MimeHandlerViewManagerHost::RemoveFrameController(
    int32_t frame_controller_id) {
  if (navigation_helpers_[frame_controller_id].state >= kDidRequestResource)
    GetRendererInterface()->RemoveFrameController(frame_controller_id);

  navigation_helpers_.erase(frame_controller_id);
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
