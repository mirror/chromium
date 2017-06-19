// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_RENDERER_MIME_HANDLER_VIEW_MIME_HANDLER_VIEW_MANAGER_
#define EXTENSIONS_RENDERER_MIME_HANDLER_VIEW_MIME_HANDLER_VIEW_MANAGER_

#include <string>
#include <unordered_map>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "content/public/renderer/render_frame_observer.h"
#include "extensions/common/mojo/mime_handler_view.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/service_manager/public/cpp/bind_source_info.h"
#include "third_party/WebKit/public/web/WebAssociatedURLLoaderClient.h"
#include "url/gurl.h"

namespace extensions {

class MimeHandlerViewClient;

class MimeHandlerViewManager
    : public content::RenderFrameObserver,
      public mime_handler_view::MimeHandlerViewManager {
 public:
  static MimeHandlerViewManager* FromRenderFrame(
      content::RenderFrame* render_frame);

  static GURL GetExtensionURL(const GURL& resource_url,
                              const std::string& mime_type);

  void DocumentLoadedInFrame(int32_t original_frame_routing_id) override;

  void MaybeRequestPDFResource(content::RenderFrame* navigating_frame,
                               const GURL& url);

  // content::RenderFrameObserver overrides.
  void OnDestruct() override;

  void ClientRenderFrameGone(int32_t id);

  MimeHandlerViewClient* GetClient(int32_t id);

 protected:
  ~MimeHandlerViewManager() override;

 private:
  MimeHandlerViewManager(content::RenderFrame* render_frame);

  void Bind(const service_manager::BindSourceInfo& source_info,
            mime_handler_view::MimeHandlerViewManagerRequest request);

  mime_handler_view::MimeHandlerViewService* GetBrowserInterface();

  void OnRegisteredMimeHandlerViewFrame(int navigating_frame_id,
                                        const std::string& pdf_url,
                                        bool request_resource);

  // Whether the frame loading the mime handler is in a PluginDocument.
  const bool is_plugin_document_;

  const int32_t owner_frame_routing_id_;

  bool did_register_interface_ = false;

  std::unordered_map<int32_t, std::unique_ptr<MimeHandlerViewClient>> clients_;

  mime_handler_view::MimeHandlerViewServicePtr browser_interface_;

  std::unique_ptr<mojo::Binding<mime_handler_view::MimeHandlerViewManager>>
      binding_;

  base::WeakPtrFactory<MimeHandlerViewManager> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(MimeHandlerViewManager);
};

}  // namespace extensions

#endif  // EXTENSIONS_RENDERER_MIME_HANDLER_VIEW_MIME_HANDLER_VIEW_MANAGER_
