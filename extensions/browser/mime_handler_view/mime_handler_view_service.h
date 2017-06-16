// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_MIME_HANDLER_VIEW_MIME_HANDLER_VIEW_SERVICE_H_
#define EXTENSIONS_BROWSER_MIME_HANDLER_VIEW_MIME_HANDLER_VIEW_SERVICE_H_

#include <map>
#include <string>

#include "base/memory/weak_ptr.h"
#include "content/public/browser/navigation_throttle.h"
#include "content/public/browser/web_contents_observer.h"
#include "extensions/browser/guest_view/mime_handler_view/mime_handler_stream_manager.h"
#include "extensions/common/mojo/mime_handler_view_service.mojom.h"
#include "services/service_manager/public/cpp/bind_source_info.h"

namespace content {
class NavigationHandle;
class RenderFrameHost;
}

namespace extensions {
class MimeHandlerStreamManager;
class StreamContainer;

// This class manages navigations to the MimeHandlerView extension. It provides
// any RenderFrameHost containing an <embed> tag the required API for tracking
// down the StreamContainer needed for MimeHandlerServiceImpl.
class MimeHandlerViewService : public mime_handler_view::MimeHandlerViewService,
                               public content::WebContentsObserver,
                               public MimeHandlerStreamManager::Observer {
 public:
  static void CreateForFrame(
      content::RenderFrameHost* render_frame_host,
      const service_manager::BindSourceInfo& bind_source_info,
      mime_handler_view::MimeHandlerViewServiceRequest request);

  static MimeHandlerViewService* FromRenderFrameHost(
      content::RenderFrameHost* render_frame_host);

  MimeHandlerViewService(content::RenderFrameHost* render_frame_host);
  ~MimeHandlerViewService() override;

  // mime_handler_view::MimeHandlerViewService overrides.
  void RegisterMimeHandlerViewFrame(
      int32_t target_frame_routing_id,
      const std::string& resource_url,
      bool in_plugin_document,
      RegisterMimeHandlerViewFrameCallback callback) override;

  // Returns true when the navigation must be deferred or blocked. A navigation
  // to PDF extension is deferred if the corresponding StreamContainer is not
  // ready.
  bool MaybeDeferNavigation(
      content::NavigationHandle* handle,
      content::NavigationThrottle::ThrottleCheckResult* result);

 private:
  struct FrameData {
    // Routing ID of the origina RenderFrameHost. Used to track StreamContainer.
    int stream_frame_routing_id;
    std::string resource_url;
    std::unique_ptr<StreamContainer> stream;
    // Non-null for the duration of the navigation to the extension.
    content::NavigationHandle* navigation_handle = nullptr;
    bool loaded = false;
  };

  // content::WebContentsObserver overrides.
  void RenderFrameHostChanged(content::RenderFrameHost* old_host,
                              content::RenderFrameHost* new_host) override;
  void RenderFrameDeleted(content::RenderFrameHost* render_frame_host) override;
  void DidFinishNavigation(content::NavigationHandle* handle) override;

  // MimeHandlerStreamManager::Observer overrides.
  void OnStreamAdded(int process_id,
                     int routing_id,
                     const std::string& view_id) override;

  std::map<int, FrameData> frames_;

  content::RenderFrameHost* const render_frame_host_;
  MimeHandlerStreamManager* const stream_manager_;
  bool did_receive_stream_for_plugin_document_ = false;

  base::WeakPtrFactory<MimeHandlerViewService> weak_factory_;
};

}  // namespace extensions
#endif  // EXTENSIONS_BROWSER_MIME_HANDLER_VIEW_MIME_HANDLER_VIEW_SERVICE_H_
