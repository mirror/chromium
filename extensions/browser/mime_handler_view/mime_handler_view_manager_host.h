// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_MIME_HANDLER_VIEW_MIME_HANDLER_VIEW_MANAGER_HOST_H_
#define EXTENSIONS_BROWSER_MIME_HANDLER_VIEW_MIME_HANDLER_VIEW_MANAGER_HOST_H_

#include <map>
#include <string>

#include "base/memory/weak_ptr.h"
#include "content/public/browser/navigation_throttle.h"
#include "content/public/browser/web_contents_observer.h"
#include "extensions/common/mojo/mime_handler_view.mojom.h"

namespace content {
class NavigationHandle;
class RenderFrameHost;
}  // namespace content

namespace extensions {
class StreamContainer;

// This class manages navigations to the MimeHandlerView extension inside an
// embedder RenderFrameHost. This excludes navigating the main frame to the
// extension URL as well as print preview. Its tasks involve (in chronological
// order):
//  * Clearing permissions for the target render frame host in PDF extension
//    process such that it can access chrome://resources.
//  * Halting the navigation to the process until the required PDF resource is
//    available as a stream.
//  * Notifying the embedder process when the PDF extension properly loads in
//    an <embed>-ed frame so that the embedder can forward posted messages to
//    the corresponding plugin element.
//
// This class's life time is bound to that of the embedder RenderFrameHost.
class MimeHandlerViewManagerHost : public content::WebContentsObserver {
 public:
  // static
  // Returns true when the navigation must be deferred or blocked. A navigation
  // to PDF extension is deferred if the corresponding StreamContainer is not
  // ready.
  static bool MaybeDeferNavigation(
      content::NavigationHandle* handle,
      content::NavigationThrottle::ThrottleCheckResult* result);

 private:
  struct NavigationHelper {
   public:
    NavigationHelper();
    ~NavigationHelper();

    int32_t original_frame_routing_id = MSG_ROUTING_NONE;
    content::NavigationHandle* navigation_handle = nullptr;
    StreamContainer* stream_container = nullptr;
  };

  static MimeHandlerViewManagerHost* Get(
      content::RenderFrameHost* embedder_frame_host);
  MimeHandlerViewManagerHost(content::RenderFrameHost* render_frame_host);
  ~MimeHandlerViewManagerHost() override;

  // content::WebContentsObserver.
  void RenderFrameHostChanged(content::RenderFrameHost* old_host,
                              content::RenderFrameHost* new_host) override;
  void RenderFrameDeleted(content::RenderFrameHost* render_frame_host) override;
  void DocumentLoadedInFrame(
      content::RenderFrameHost* render_frame_host) override;

  void InterceptNavigation(content::NavigationHandle* handle);
  mime_handler_view::MimeHandlerViewManager* GetRendererInterface();

  std::map<int32_t, NavigationHelper> navigation_helpers_;

  content::RenderFrameHost* const render_frame_host_;
  mime_handler_view::MimeHandlerViewManagerPtr renderer_interface_;
  base::WeakPtrFactory<MimeHandlerViewManagerHost> weak_factory_;
};

}  // namespace extensions
#endif  // EXTENSIONS_BROWSER_MIME_HANDLER_VIEW_MIME_HANDLER_VIEW_MANAGER_HOST_H_
