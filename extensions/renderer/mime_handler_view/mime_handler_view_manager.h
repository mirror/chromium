// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_RENDERER_MIME_HANDLER_VIEW_MIME_HANDLER_VIEW_MANAGER_H_
#define EXTENSIONS_RENDERER_MIME_HANDLER_VIEW_MIME_HANDLER_VIEW_MANAGER_H_

#include "base/macros.h"
#include "content/public/renderer/render_frame_observer.h"
#include "extensions/common/mojo/mime_handler_view.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/service_manager/public/cpp/bind_source_info.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "third_party/WebKit/public/platform/WebURL.h"

namespace blink {
class WebElement;
}  // namespace blink

namespace content {
class RenderFrame;
}  // namespace content

namespace v8 {
template <class F>
class Local;
class Isolate;
class Object;
}  // namespace v8

namespace extensions {
class MimeHandlerViewFrameController;

// This class manages embedded mime types which are handled by an extension
// inside a given parent frame.
class MimeHandlerViewManager
    : public content::RenderFrameObserver,
      public mime_handler_view::MimeHandlerViewManager {
 public:
  // Returns the instance of MimeHanlderViewManager associated with
  // |render_frame|. All <embed>-ed PDFs inside |render_frame| are managed with
  // this instance of MimeHandlerViewManager.
  static MimeHandlerViewManager* Get(content::RenderFrame* render_frame);

  // Binding method for the RenderFrame associated with
  // |render_frame_routing_id|.
  static void CreateInterface(
      int32_t render_frame_routing_id,
      mime_handler_view::MimeHandlerViewManagerRequest request);

  // Returns true if the mime-type can be handled by an extension handler and
  // loads a simple HTML page with <embed> tag inside |render_frame|. The
  // <embed> tag will use |url| as its 'src' attribute.
  static bool OverrideNavigationForMimeHandlerView(
      content::RenderFrame* render_frame,
      const GURL& url,
      const std::string& actual_mime_type);

  // content::RenderFrameObserver overrides.
  void DidCommitProvisionalLoad(
      bool unused_is_new_navigation,
      bool unused_is_same_document_navigation) override;
  void OnDestruct() override;

  // mime_handler_view::MimeHandlerViewManager overrides.
  void DocumentLoaded(const std::string& stream_id) override;
  void DidAbortStream(const std::string& stream_id) override;
  void FrameRemoved(const std::string& stream_id) override;

  // The following methods are plumbed from blink/core/html.
  //
  // Returns true if |resource_url| and |mime_type| can be handled and a
  // controller is created. Returns false otherwise.
  bool CreatePluginController(const blink::WebElement& owner,
                              const GURL& resource_url,
                              const std::string& mime_type);
  v8::Local<v8::Object> CreateV8ScriptableObject(const blink::WebElement& owner,
                                                 v8::Isolate* isolate);

  // The following methods are called by MimeHandlerViewFrameControllers managed
  // by this manager.
  //
  // Called by MimeHandlerViewFrameController to inform that the StreamContainer
  // on the browser side is ready and the controller has received its
  // |stream_id|.
  void DidReceiveStreamId(MimeHandlerViewFrameController* controller);

  // Called when loading the PDF resource fails for any reason. This means there
  // will not be a |stream_id| and therefore, the navigation should not occur.
  void DidFailLoadingResource(MimeHandlerViewFrameController* controller);

 private:
  explicit MimeHandlerViewManager(content::RenderFrame* render_frame);
  ~MimeHandlerViewManager() override;

  MimeHandlerViewFrameController* GetFrameController(
      const blink::WebElement& owner);
  MimeHandlerViewFrameController* GetFrameController(
      const std::string& stream_id);
  mime_handler_view::MimeHandlerViewManagerHost* GetBrowserInterface();

  const int32_t render_frame_routing_id_;
  std::list<std::unique_ptr<MimeHandlerViewFrameController>> frame_controllers_;
  mojo::Binding<mime_handler_view::MimeHandlerViewManager> binding_;
  mime_handler_view::MimeHandlerViewManagerHostPtr browser_interface_;

  DISALLOW_COPY_AND_ASSIGN(MimeHandlerViewManager);
};

}  // namespace extensions

#endif  // EXTENSIONS_RENDERER_MIME_HANDLER_VIEW_MIME_HANDLER_VIEW_MANAGER_H_
