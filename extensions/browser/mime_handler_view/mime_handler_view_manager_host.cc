// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/mime_handler_view/mime_handler_view_manager_host.h"

#include "base/feature_list.h"
#include "base/lazy_instance.h"
#include "content/public/browser/child_process_security_policy.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_features.h"
#include "extensions/browser/api/mime_handler_private/mime_handler_private.h"
#include "extensions/browser/extension_util.h"
#include "extensions/browser/guest_view/mime_handler_view/mime_handler_view_guest.h"
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
  Get(parent_frame)->InterceptNavigation(handle);
  *result = content::NavigationThrottle::DEFER;

  // TODO(ekaramad): Implement the logic to defer the navigation and for stream
  // for PDF resource to be ready.
  return true;
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
      weak_factory_(this) {
  g_render_frame_map.Get().insert({render_frame_host, this});

  if (auto* manager = GetStreamManager()) {
    manager->AddObserver(this);
  }
}

MimeHandlerViewManagerHost::~MimeHandlerViewManagerHost() {
  g_render_frame_map.Get().erase(render_frame_host_);
  if (auto* manager = GetStreamManager())
    manager->RemoveObserver(this);
}

void MimeHandlerViewManagerHost::RenderFrameHostChanged(
    content::RenderFrameHost* old_host,
    content::RenderFrameHost* new_host) {
  int node_id = new_host->GetFrameTreeNodeId();
  if (!navigation_helpers_.count(node_id))
    return;

  if (!util::IsPdfExtensionUrl(new_host->GetSiteInstance()->GetSiteURL())) {
    // The frame is not going to the PDF extension process so we should not
    // grant any permissions needed for PDF extension. The local state will be
    // released when the |old_host| is deleted.
    return;
  }

  if (auto* stream_container =
          navigation_helpers_[node_id].stream_container.get()) {
    // Expose the MimeHandlerServiceImpl API to the target frame.
    new_host->GetInterfaceRegistry()->AddInterface(base::Bind(
        &MimeHandlerServiceImpl::Create, stream_container->GetWeakPtr()));
  }

  navigation_helpers_[node_id].navigation_handle = nullptr;
  navigation_helpers_[node_id].state = kTargetFrameLoading;
}

void MimeHandlerViewManagerHost::RenderFrameDeleted(
    content::RenderFrameHost* render_frame_host) {
  int node_id = render_frame_host->GetFrameTreeNodeId();
  if (!navigation_helpers_.count(node_id))
    return;

  if (navigation_helpers_[node_id].state == kTargetFrameLoading) {
    // This is a legitimate case for deleting the original RenderFrameHost
    // (i.e., when the corresponding content frame of the <embed> swaps out).
    DCHECK_EQ(navigation_helpers_[node_id].original_frame_routing_id,
              render_frame_host->GetRoutingID());
    return;
  }

  // We should notify the embedder process.
  if (auto* interface = GetRendererInterface())
    interface->FrameDeleted(node_id);

  navigation_helpers_.erase(node_id);
}

void MimeHandlerViewManagerHost::DocumentLoadedInFrame(
    content::RenderFrameHost* render_frame_host) {
  int node_id = render_frame_host->GetFrameTreeNodeId();
  if (!navigation_helpers_.count(node_id))
    return;

  DCHECK_EQ(render_frame_host->GetParent(), render_frame_host_);
  DCHECK(!navigation_helpers_[node_id].navigation_handle);
  DCHECK(util::IsPdfExtensionUrl(
      render_frame_host->GetSiteInstance()->GetSiteURL()));
  DCHECK(GetRendererInterface());
  GetRendererInterface()->DocumentLoaded(
      navigation_helpers_[node_id].original_frame_routing_id);
  navigation_helpers_[node_id].state = kDocumentLoaded;
}

void MimeHandlerViewManagerHost::OnStreamAdded(int32_t process_id,
                                               int32_t frame_tree_node_id,
                                               const std::string& view_id) {
  if (process_id != render_frame_host_->GetProcess()->GetID())
    return;

  content::RenderFrameHost* rfh = web_contents()->FindFrameByFrameTreeNodeId(
      frame_tree_node_id, process_id);
  if (!rfh)
    return;

  if (rfh == render_frame_host_) {
    // TODO(ekaramad):When a frame is navigated to a PDF resource, a request is
    // automatically made and the stream for the parent frame is intercepted.
    // For simplicity, we drop this now and request again through the content
    // frame of the <embed>. Fix this behavior my making all such requests on
    // the browser side.
    auto aborted_stream = GetStreamManager()->ReleaseStream(view_id);
    return;
  }

  if (!navigation_helpers_.count(frame_tree_node_id))
    return;

  std::unique_ptr<StreamContainer> stream_container =
      GetStreamManager()->ReleaseStream(view_id);

  if (stream_container) {
    navigation_helpers_[frame_tree_node_id].stream_container =
        std::move(stream_container);
    navigation_helpers_[frame_tree_node_id].navigation_handle->Resume();
    if (navigation_helpers_[frame_tree_node_id].same_origin) {
      rfh->GetInterfaceRegistry()->AddInterface(base::Bind(
          &MimeHandlerServiceImpl::Create, stream_container->GetWeakPtr()));
    }
  } else {
    navigation_helpers_[frame_tree_node_id]
        .navigation_handle->CancelDeferredNavigation(
            content::NavigationThrottle::CANCEL_AND_IGNORE);
  }
}

void MimeHandlerViewManagerHost::InterceptNavigation(
    content::NavigationHandle* handle) {
  int32_t node_id = handle->GetFrameTreeNodeId();
  navigation_helpers_[node_id].original_frame_routing_id =
      web_contents()
          ->FindFrameByFrameTreeNodeId(
              node_id, render_frame_host_->GetProcess()->GetID())
          ->GetRoutingID();
  navigation_helpers_[node_id].navigation_handle = handle;
  navigation_helpers_[node_id].resource_url = handle->GetURL().query();
  if (util::IsPdfExtensionURL(handle->GetPreviousURL())) {
    // The frame is being navigated to the PDF extension again. This could be
    // done by either changing window.location or changing the source attribute.
    navigation_helpers_[node_id].same_origin = true;
  }

  // We should kick off a resource loading request.
  // TODO(ekaramad): Can we just send this request on the browser side?
  if (auto* interface = GetRendererInterface()) {
    interface->RequestResource(
        navigation_helpers_[node_id].original_frame_routing_id,
        navigation_helpers_[node_id].resource_url);
    navigation_helpers_[node_id].state = kDidRequestResource;
  }
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
