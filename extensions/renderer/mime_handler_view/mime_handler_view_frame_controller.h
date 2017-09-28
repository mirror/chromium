// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_RENDERER_MIME_HANDLER_VIEW_MIME_HANDLER_VIEW_FRAME_CONTROLLER_H_
#define EXTENSIONS_RENDERER_MIME_HANDLER_VIEW_MIME_HANDLER_VIEW_FRAME_CONTROLLER_H_

#include "base/memory/weak_ptr.h"
#include "base/strings/stringprintf.h"
#include "ipc/ipc_message.h"
#include "third_party/WebKit/public/platform/WebURL.h"
#include "third_party/WebKit/public/web/WebAssociatedURLLoaderClient.h"
#include "third_party/WebKit/public/web/WebMimeHandlerViewManager.h"
#include "url/gurl.h"
#include "v8/include/v8.h"

namespace blink {
class WebAssociatedURLLoader;
struct WebURLError;
}  // namespace blink

namespace extensions {
class MimeHandlerViewManager;

class MimeHandlerViewFrameController
    : public blink::WebAssociatedURLLoaderClient {
 public:
  MimeHandlerViewFrameController(blink::WebMimeHandlerViewManager::HandlerId id,
                                 const GURL& resource_url,
                                 MimeHandlerViewManager* manager);
  explicit MimeHandlerViewFrameController(MimeHandlerViewManager* manager);
  ~MimeHandlerViewFrameController() override;

  // These methods are called by MimeHandlerViewManager.
  void DidPauseNavigation(int32_t frame_tree_node_id);
  void SetId(int32_t id);
  void RequestResource();
  void DocumentLoaded();

  v8::Local<v8::Object> V8ScriptableObject(v8::Isolate* isolate);

  // Post a JavaScript message to the guest.
  void PostMessage(v8::Isolate* isolate, v8::Local<v8::Value> message);

  // WebAssociatedURLLoaderClient overrides.
  void DidReceiveData(const char* data, int data_length) override;
  void DidFinishLoading(double finish_time) override;
  void DidFail(const blink::WebURLError&) override;

  const std::string& view_id() const { return view_id_; }
  int32_t frame_tree_node_id() const { return frame_tree_node_id_; }
  const GURL& resource_url() const { return resource_url_; }

 private:
  // Used to communicate with blink.
  blink::WebMimeHandlerViewManager::HandlerId id_;

  // Used to communicate with the browser process.
  int32_t frame_tree_node_id_;

  // The URL of the PDF resource. This might change during the lifetime of the
  // controller.
  const GURL resource_url_;

  MimeHandlerViewManager* const manager_;

  // Used to request the PDF resource when <embed> is not the first in a
  // PluginDocument.
  std::unique_ptr<blink::WebAssociatedURLLoader> loader_;

  // |view_id| is a GUID which will uniquely identify the StreamContainer on the
  // browser side.
  std::string view_id_;
  bool did_receive_view_id_ = false;

  // The scriptable object used to provide custom API for the plugin element.
  v8::Global<v8::Object> scriptable_object_;

  // A queue of all the posted messages which will be emptied when the frame
  // loads the PDF extension. Any message sent to the plugin before that is
  // stored here.
  std::vector<v8::Global<v8::Value>> pending_messages_;

  bool document_loaded_ = false;

  base::WeakPtrFactory<MimeHandlerViewFrameController> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(MimeHandlerViewFrameController);
};
}  // namespace extensions

#endif  // EXTENSIONS_RENDERER_MIME_HANDLER_VIEW_MIME_HANDLER_VIEW_FRAME_CONTROLLER_H_
