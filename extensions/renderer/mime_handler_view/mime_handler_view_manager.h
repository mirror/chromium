// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_RENDERER_MIME_HANDLER_VIEW_MIME_HANDLER_VIEW_MANAGER_H_
#define EXTENSIONS_RENDERER_MIME_HANDLER_VIEW_MIME_HANDLER_VIEW_MANAGER_H_

#include "base/macros.h"
#include "extensions/common/mojo/mime_handler_view.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/service_manager/public/cpp/bind_source_info.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "third_party/WebKit/public/platform/WebURL.h"
#include "third_party/WebKit/public/web/WebMimeHandlerViewManager.h"

namespace content {
class RenderFrame;
}  // namespace content

namespace v8 {
template <class F>
class Local;
class Isolate;
class Object;
class WebString;
}  // namespace v8

namespace extensions {
class MimeHandlerViewFrameController;

// This class manages embedded mime types which are handled by an extension
// inside a given parent frame.
class MimeHandlerViewManager
    : public blink::WebMimeHandlerViewManager,
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

  // blink::WebMimeHandlerViewManager overrides.
  HandlerId CreateHandler(const blink::WebURL& resource_url,
                          const blink::WebString& mime_type) override;

  blink::WebURL GetUrl(HandlerId id) const override;
  v8::Local<v8::Object> V8ScriptableObject(HandlerId id,
                                           v8::Isolate* isolate) override;
  void DidReceiveDataInPluginDocument(const char* data,
                                      size_t data_length) override;

  // mime_handler_view::MimeHandlerViewManager overrides.
  void DidPauseNavigation(int32_t render_frame_routing_id,
                          int32_t frame_tree_node_id) override;
  void DocumentLoaded(int32_t frame_tree_node_id) override;
  void FrameRemoved(int32_t frame_tree_node_id) override;

  // Called by MimeHandlerViewFrameController to inform that the StreamContainer
  // on the browser side is ready and the controller has received its |view_id|.
  void DidReceiveViewId(HandlerId id);
  // Called when loading the PDF resource fails for any reason. This means there
  // will not be a |view_id| and therefore, the navigation should not occur.
  void DidFailLoadingResource(HandlerId id);

 private:
  explicit MimeHandlerViewManager(blink::WebLocalFrame* owner_frame);
  ~MimeHandlerViewManager() override;

  HandlerId GetHandlerIdFromFrameTreeNodeId(int32_t frame_tree_node_id);
  mime_handler_view::MimeHandlerViewManagerHost* GetBrowserInterface();
  void RemoveFrameController(HandlerId id);

  std::map<HandlerId, std::unique_ptr<MimeHandlerViewFrameController>>
      frame_controller_map_;
  mojo::Binding<mime_handler_view::MimeHandlerViewManager> binding_;
  mime_handler_view::MimeHandlerViewManagerHostPtr browser_interface_;

  DISALLOW_COPY_AND_ASSIGN(MimeHandlerViewManager);
};

}  // namespace extensions

#endif  // EXTENSIONS_RENDERER_MIME_HANDLER_VIEW_MIME_HANDLER_VIEW_MANAGER_H_
