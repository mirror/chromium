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

  if (util::IsPdfExtensionUrl(handle->GetURL()))
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
  return false;
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
}

MimeHandlerViewManagerHost::~MimeHandlerViewManagerHost() {
  g_render_frame_map.Get().erase(render_frame_host_);
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

  navigation_helpers_[node_id].navigation_handle = nullptr;
}

void MimeHandlerViewManagerHost::RenderFrameDeleted(
    content::RenderFrameHost* render_frame_host) {
  int node_id = render_frame_host->GetFrameTreeNodeId();
  if (!navigation_helpers_.count(node_id))
    return;

  if (navigation_helpers_[node_id].navigation_handle) {
    DCHECK_EQ(navigation_helpers_[node_id].original_frame_routing_id,
              render_frame_host->GetRoutingID());
    // This is a case where the plugin's content frame is deleted while we have
    // a deferred navigation (waiting for stream). We should cancel the deferred
    // navigation.
    navigation_helpers_[node_id].navigation_handle->CancelDeferredNavigation(
        content::NavigationThrottle::CANCEL_AND_IGNORE);
    navigation_helpers_.erase(node_id);
  } else {
    // Navigation had already committed and the frame was swapped.
    DCHECK_NE(navigation_helpers_[node_id].original_frame_routing_id,
              render_frame_host->GetRoutingID());
    navigation_helpers_.erase(node_id);
  }
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

  // We should kick off a resource loading request.
  // TODO(ekaramad): Can we just send this request on the browser side?
  if (auto* interface = GetRendererInterface()) {
    interface->RequestResource(
        navigation_helpers_[node_id].original_frame_routing_id);
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

MimeHandlerViewManagerHost::NavigationHelper::NavigationHelper() {}
MimeHandlerViewManagerHost::NavigationHelper::~NavigationHelper() {}

}  // namespace extensions
