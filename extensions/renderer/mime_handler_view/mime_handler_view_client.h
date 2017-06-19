// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_RENDERER_MIME_HANDLER_VIEW_MIME_HANDLER_VIEW_CLIENT_
#define EXTENSIONS_RENDERER_MIME_HANDLER_VIEW_MIME_HANDLER_VIEW_CLIENT_

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "content/public/renderer/render_frame_observer.h"
#include "extensions/common/mojo/mime_handler_view.mojom.h"
#include "third_party/WebKit/public/platform/WebURL.h"
#include "third_party/WebKit/public/web/WebAssociatedURLLoaderClient.h"
#include "url/gurl.h"
#include "v8/include/v8.h"

namespace blink {
class WebAssociatedURLLoader;
class WebURL;
}  // namespace blink

namespace content {
class RenderFrame;
}

namespace extensions {

class MimeHandlerViewManager;

// This class helps with embedding MimeHandlerView extension and provides post
// messaging API as well as resource requests for the mime type. It is owned by
// MimeHandlerViewManage of the embedder frame.
class MimeHandlerViewClient : public blink::WebAssociatedURLLoaderClient,
                              public content::RenderFrameObserver {
 public:
  MimeHandlerViewClient(content::RenderFrame* original_render_frame,
                        const GURL& pdf_url,
                        bool send_request);
  ~MimeHandlerViewClient() override;

  void DocumentLoadedInFrame();

  bool resource_request_complete() const { return resource_request_complete_; }

  // Returns the implementation of "postMessage" API.
  v8::Local<v8::Object> GetV8ScriptingObject(
      v8::Isolate* isolate,
      v8::Local<v8::Object> global_proxy);

 private:
  // blink::WebAssociatedURLLoaderClient overrides.
  void DidFinishLoading(double time) override;

  // content::RenderFrameObserver overrides.
  void OnDestruct() override;

  void SendRequest();

  // Post a JavaScript message to the guest.
  void PostMessage(v8::Isolate* isolate, v8::Local<v8::Value> message);

  // The scriptable object that backs the plugin.
  v8::Global<v8::Object> scriptable_object_;

  // The GlobalProxy associated with the frame which is navigated to the
  // extension. This is used to implement post messaging API.
  v8::Local<v8::Object> global_proxy_;

  // When true we can post messages to the extension. Otherwise any incoming
  // message will be queued up.
  bool document_loaded_in_main_frame_ = false;

  // This is the routing ID of the original frame which is then navigated to the
  // extension. The frame will be destroyed during the process and this ID is
  // only used to identify and communicate with the right MimeHandlerViewClient.
  const int32_t original_frame_routing_id_;

  bool resource_request_complete_ = false;

  // The URL to the PDF resource.
  const blink::WebURL pdf_url_;

  MimeHandlerViewManager* const mime_handler_view_manager_;

  // Pending postMessage messages that need to be sent to the guest. These are
  // queued while the guest is loading and once it is fully loaded they are
  // delivered so that messages aren't lost.
  std::vector<v8::Global<v8::Value>> pending_messages_;

  std::unique_ptr<blink::WebAssociatedURLLoader> loader_;

  base::WeakPtrFactory<MimeHandlerViewClient> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(MimeHandlerViewClient);
};

}  // namespace extensions.

#endif  // EXTENSIONS_RENDERER_MIME_HANDLER_VIEW_MIME_HANDLER_VIEW_CLIENT_
