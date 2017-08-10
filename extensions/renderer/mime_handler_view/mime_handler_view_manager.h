// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_RENDERER_MIME_HANDLER_VIEW_MIME_HANDLER_VIEW_MANAGER_H_
#define EXTENSIONS_RENDERER_MIME_HANDLER_VIEW_MIME_HANDLER_VIEW_MANAGER_H_

#include "base/macros.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_frame_observer.h"
#include "extensions/common/mojo/mime_handler_view.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/service_manager/public/cpp/bind_source_info.h"
#include "services/service_manager/public/cpp/interface_provider.h"

namespace blink {
class WebFrame;
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
class MimeHandlerViewManager : public mime_handler_view::MimeHandlerViewManager,
                               public content::RenderFrameObserver {
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

  // mime_handler_view::MimeHandlerViewManager overrides.
  void RequestResource(int32_t frame_controller_id,
                       int32_t routing_id,
                       const std::string& resource_url) override;
  void DocumentLoaded(int32_t frame_controller_id) override;
  void RemoveFrameController(int32_t frame_controller_id) override;
  void Destroy() override;

  // content::RenderFrameObserver overrides.
  void OnDestruct() override;
  void ChildFrameWillSwap(blink::WebFrame* old_frame,
                          blink::WebFrame* new_frame) override;

  void SetViewId(int32_t frame_controller_id, const std::string& view_id);

  v8::Local<v8::Object> GetV8ScriptableObjectForPluginFrame(
      v8::Isolate* isolate,
      blink::WebFrame* web_frame);

 private:
  explicit MimeHandlerViewManager(content::RenderFrame* render_frame);
  ~MimeHandlerViewManager() override;

  MimeHandlerViewFrameController* GetFrameController(
      blink::WebFrame* web_frame);
  mime_handler_view::MimeHandlerViewManagerHost* GetBrowserInterface();

  const int32_t internal_map_key_;
  std::map<int32_t, std::unique_ptr<MimeHandlerViewFrameController>>
      frame_controller_map_;
  std::map<int32_t, std::unique_ptr<MimeHandlerViewFrameController>>
      pending_frame_controller_map_;
  mojo::Binding<mime_handler_view::MimeHandlerViewManager> binding_;
  mime_handler_view::MimeHandlerViewManagerHostPtr browser_interface_;

  DISALLOW_COPY_AND_ASSIGN(MimeHandlerViewManager);
};

}  // namespace extensions

#endif  // EXTENSIONS_RENDERER_MIME_HANDLER_VIEW_MIME_HANDLER_VIEW_MANAGER_H_
