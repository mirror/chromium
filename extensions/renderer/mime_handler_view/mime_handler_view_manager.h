// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_RENDERER_MIME_HANDLER_VIEW_MIME_HANDLER_VIEW_MANAGER_
#define EXTENSIONS_RENDERER_MIME_HANDLER_VIEW_MIME_HANDLER_VIEW_MANAGER_

#include <set>
#include <string>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "content/public/renderer/render_frame_observer.h"
#include "extensions/common/mojo/mime_handler_view_service.mojom.h"
#include "third_party/WebKit/public/web/WebAssociatedURLLoaderClient.h"
#include "url/gurl.h"

namespace blink {
class WebAssociatedURLLoader;
class WebURL;
}  // namespace blink

namespace extensions {

class MimeHandlerViewManager : public content::RenderFrameObserver {
 public:
  static MimeHandlerViewManager* FromRenderFrame(
      content::RenderFrame* render_frame);

  static GURL GetExtensionURL(const GURL& resource_url,
                              const std::string& mime_type);

  void MaybeRequestResource(content::RenderFrame* navigating_frame,
                            const GURL& url);

  // content::RenderFrameObserver overrides.
  void OnDestruct() override;

 protected:
  ~MimeHandlerViewManager() override;

 private:
  class ResourceRequestClient : public blink::WebAssociatedURLLoaderClient {
   public:
    ResourceRequestClient(MimeHandlerViewManager* manager,
                          content::RenderFrame* render_frame,
                          const blink::WebURL& url);

    ~ResourceRequestClient() override;

   private:
    // blink::WebAssociatedURLLoaderClient overrides.
    void DidFinishLoading(double time) override;

    MimeHandlerViewManager* const manager_;
    std::unique_ptr<blink::WebAssociatedURLLoader> loader_;
  };

  friend class ResourceRequestClient;

  MimeHandlerViewManager(content::RenderFrame* render_frame);

  void DidFinishLoadingResource(ResourceRequestClient* client);

  mime_handler_view::MimeHandlerViewService* GetBrowserInterface();

  void OnRegisteredMimeHandlerViewFrame(const std::string& pdf_url,
                                        int navigating_frame_id,
                                        bool request_resource);

  // The MIME type of the plugin.
  const std::string mime_type_;

  // The URL of the extension to navigate to.
  std::string view_id_;

  // Whether the frame loading the mime handler is in a PluginDocument.
  const bool is_plugin_document_;

  // The original URL of the resource.
  GURL original_url_;

  std::set<ResourceRequestClient*> pending_requests_;

  mime_handler_view::MimeHandlerViewServicePtr browser_interface_;

  base::WeakPtrFactory<MimeHandlerViewManager> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(MimeHandlerViewManager);
};

}  // namespace extensions

#endif  // EXTENSIONS_RENDERER_MIME_HANDLER_VIEW_MIME_HANDLER_VIEW_MANAGER_
