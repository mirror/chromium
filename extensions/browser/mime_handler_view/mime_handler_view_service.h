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
#include "extensions/common/mojo/mime_handler_view.mojom.h"
#include "services/service_manager/public/cpp/bind_source_info.h"

namespace content {
class NavigationHandle;
class RenderFrameHost;
}  // namespace content

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
    FrameData();
    ~FrameData();

    // The original frame which is navigating to PDF extension. It will be
    // destoryed during the process.
    int32_t original_frame_routing_id = MSG_ROUTING_NONE;

    // Routing ID of the frame which sends the request for loading the PDF
    // resource. It is either the frame which is also navigating to the
    // extension, or the embedder frame.
    int stream_frame_routing_id = MSG_ROUTING_NONE;

    // The original resource URL. This is the URL to the PDF resource which is
    // provided in the omnibox or as the 'src' attributed of an <embed>/<object>
    // element.
    std::string resource_url;

    // The stream container which is used in providing the MimHandlerServiceImpl
    // API to the extension process.
    std::unique_ptr<StreamContainer> stream;

    // Non-null for the duration of the navigation to the extension.
    content::NavigationHandle* navigation_handle = nullptr;

    // True when navigation to the extension process ends.
    bool did_finish_navigation = false;

    // True in the final stage when the extension document fully loads. At this
    // point, the renderer in the embedder side should be notified so that post
    // messageing API is activated.
    bool document_loaded = false;
  };

  // content::WebContentsObserver overrides.
  void RenderFrameHostChanged(content::RenderFrameHost* old_host,
                              content::RenderFrameHost* new_host) override;
  void RenderFrameDeleted(content::RenderFrameHost* render_frame_host) override;
  void DidFinishNavigation(content::NavigationHandle* handle) override;
  void DocumentLoadedInFrame(
      content::RenderFrameHost* render_frame_host) override;

  // MimeHandlerStreamManager::Observer overrides.
  void OnStreamAdded(int process_id,
                     int routing_id,
                     const std::string& view_id) override;

  mime_handler_view::MimeHandlerViewManager* GetRendererInterface();

  std::map<int, FrameData> frames_;

  content::RenderFrameHost* const render_frame_host_;
  MimeHandlerStreamManager* const stream_manager_;
  bool did_receive_stream_for_plugin_document_ = false;
  mime_handler_view::MimeHandlerViewManagerPtr renderer_interface_;

  base::WeakPtrFactory<MimeHandlerViewService> weak_factory_;
};

}  // namespace extensions
#endif  // EXTENSIONS_BROWSER_MIME_HANDLER_VIEW_MIME_HANDLER_VIEW_SERVICE_H_
