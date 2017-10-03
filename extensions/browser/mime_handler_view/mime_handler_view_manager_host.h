// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_MIME_HANDLER_VIEW_MIME_HANDLER_VIEW_MANAGER_HOST_H_
#define EXTENSIONS_BROWSER_MIME_HANDLER_VIEW_MIME_HANDLER_VIEW_MANAGER_HOST_H_

#include <string>
#include <unordered_map>

#include "base/memory/weak_ptr.h"
#include "content/public/browser/navigation_throttle.h"
#include "content/public/browser/web_contents_observer.h"
#include "extensions/browser/guest_view/mime_handler_view/mime_handler_stream_manager.h"
#include "extensions/common/mojo/mime_handler_view.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/system/message_pipe.h"
#include "services/service_manager/public/cpp/binder_registry.h"

using content::RenderFrameHost;

namespace content {
class NavigationHandle;
}  // namespace content

namespace extensions {
class StreamContainer;

// This class manages navigations to the MimeHandlerView extension inside an
// embedder RenderFrameHost. This excludes navigating the main frame to the
// extension URL as well as print preview. Its tasks involve (in chronological
// order):
//  * Clearing permissions for the target RenderFrameHost in PDF extension
//    process such that it can access chrome://resources.
//  * Halting the navigation to the process until the required PDF resource is
//    available as a stream.
//  * Notifying the embedder process when the PDF extension properly loads in
//    an <embed>-ed frame so that the embedder can forward posted messages to
//    the corresponding plugin element.
//
// This class's life time is bound to that of the embedder RenderFrameHost.
class MimeHandlerViewManagerHost
    : public content::WebContentsObserver,
      public mime_handler_view::MimeHandlerViewManagerHost {
 public:
  static void CreateInterface(
      mime_handler_view::MimeHandlerViewManagerHostRequest request,
      RenderFrameHost* rfh);

  static MimeHandlerViewManagerHost* Get(RenderFrameHost* embedder_frame_host);

 private:
  enum NavigationState {
    // Initial default state (invalid for all navigations).
    kDefault,
    // A navigation has been intercepted and a resource request has been sent.
    // We are now waiting for the stream to be intercepted.
    kDidPauseNavigation,
    // Stream is intercepted and navigation handle has resumed but the
    // RenderFrameHost has not swapped yet.
    kDidResumeNavigation,
    // The target RenderFrameHost is now loading the extension but the document
    // has not fully loaded yet.
    kTargetFrameLoading,
    // The document for extension has loaded in its RenderFrameHost.
    kDidLoadDocument,
  };

  // A simple struct which keeps track of a navigation to PDF extension.
  struct ControllerInfo {
    ControllerInfo();
    ~ControllerInfo();

    // The routing ID of the original plugin frame which is used to communicate
    // with the right controller on the renderer side.
    int32_t original_render_frame_routing_id = MSG_ROUTING_NONE;

    // The stream used to provide the MimeHandlerService API to PDF viewer
    // extension.
    std::unique_ptr<StreamContainer> stream_container;
    // The RenderFrameHost which is rendering the PDF extension.
    RenderFrameHost* destination_render_frame_host = nullptr;
  };

  MimeHandlerViewManagerHost(RenderFrameHost* render_frame_host);
  ~MimeHandlerViewManagerHost() override;

  // content::WebContentsObserver.
  void RenderFrameDeleted(RenderFrameHost* render_frame_host) override;
  void ReadyToCommitNavigation(
      content::NavigationHandle* navigation_handle) override;
  void DidFinishNavigation(content::NavigationHandle* handle) override;
  void DocumentLoadedInFrame(RenderFrameHost* render_frame_host) override;
  void OnInterfaceRequestFromFrame(
      content::RenderFrameHost* render_frame_host,
      const std::string& interface_name,
      mojo::ScopedMessagePipeHandle* interface_pipe) override;

  // mime_handler_view::MimeHandlerViewManagerHost overrides.
  void ReleaseStream(int32_t frame_controller_id,
                     const std::string& view_id) override;

 private:
  void AddMimeHandlerServiceInterface(int frame_tree_node_id);
  void RemoveFrameState(int32_t frame_tree_node_id);
  mime_handler_view::MimeHandlerViewManager* GetRendererInterface();
  MimeHandlerStreamManager* GetStreamManager();

  std::unordered_map<int32_t, ControllerInfo> controller_map_;

  RenderFrameHost* const render_frame_host_;
  mojo::Binding<mime_handler_view::MimeHandlerViewManagerHost> binding_;
  mime_handler_view::MimeHandlerViewManagerPtr renderer_interface_;
  std::unordered_map<int32_t, service_manager::BinderRegistry> registry_map_;

  base::WeakPtrFactory<MimeHandlerViewManagerHost> weak_factory_;
};

}  // namespace extensions
#endif  // EXTENSIONS_BROWSER_MIME_HANDLER_VIEW_MIME_HANDLER_VIEW_MANAGER_HOST_H_
