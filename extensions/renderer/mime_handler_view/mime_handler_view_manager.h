// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_RENDERER_MIME_HANDLER_VIEW_MIME_HANDLER_VIEW_MANAGER_H_
#define EXTENSIONS_RENDERER_MIME_HANDLER_VIEW_MIME_HANDLER_VIEW_MANAGER_H_

#include "base/macros.h"
#include "content/public/renderer/render_frame.h"
#include "extensions/common/mojo/mime_handler_view.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/service_manager/public/cpp/bind_source_info.h"

namespace content {
class RenderFrame;
}  // namespace content

namespace extensions {
class MimeHandlerViewFrameController;

// This class manages embedded mime types which are handled by an extension
// inside a given parent frame.
class MimeHandlerViewManager
    : public mime_handler_view::MimeHandlerViewManager {
 public:
  // Returns the instance of MimeHanlderViewManager associated with
  // |render_frame|. All <embed>-ed PDFs inside |render_frame| are managed with
  // this instance of MimeHandlerViewManager.
  static MimeHandlerViewManager* Get(content::RenderFrame* render_frame);

  // Binding method for the RenderFrame associated with
  // |render_frame_routing_id|.
  static void CreateInterface(
      int32_t render_frame_routing_id,
      const service_manager::BindSourceInfo& source_info,
      mime_handler_view::MimeHandlerViewManagerRequest request);

  void RequestResource(int32_t client_id,
                       const std::string& resource_url) override;
  void DocumentLoaded(int32_t client_id) override;

 private:
  explicit MimeHandlerViewManager(content::RenderFrame* render_frame);
  ~MimeHandlerViewManager() override;

  void CreateInterfaceInternal(
      mime_handler_view::MimeHandlerViewManagerRequest request);

  content::RenderFrame* const render_frame_;
  std::map<int32_t, std::unique_ptr<MimeHandlerViewFrameController>>
      frame_controller_map_;
  mojo::Binding<mime_handler_view::MimeHandlerViewManager> binding_;

  DISALLOW_COPY_AND_ASSIGN(MimeHandlerViewManager);
};

}  // namespace extensions

#endif  // EXTENSIONS_RENDERER_MIME_HANDLER_VIEW_MIME_HANDLER_VIEW_MANAGER_H_
