// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_RENDERER_MIME_HANDLER_VIEW_MIME_HANDLER_VIEW_FRAME_CONTROLLER_H_
#define EXTENSIONS_RENDERER_MIME_HANDLER_VIEW_MIME_HANDLER_VIEW_FRAME_CONTROLLER_H_

#include "base/memory/weak_ptr.h"
#include "third_party/WebKit/public/platform/WebURL.h"
#include "third_party/WebKit/public/web/WebAssociatedURLLoaderClient.h"
#include "url/gurl.h"
#include "v8/include/v8.h"

namespace blink {
class WebAssociatedURLLoader;
class WebFrame;
}  // namespace blink

namespace extensions {
class MimeHandlerViewManager;

class MimeHandlerViewFrameController
    : public blink::WebAssociatedURLLoaderClient {
 public:
  explicit MimeHandlerViewFrameController(MimeHandlerViewManager* manager);
  ~MimeHandlerViewFrameController() override;

  // These methods are called by MimeHandlerViewManager.
  void SetId(int32_t unique_id_);
  void RequestResource(const std::string& resource_url);
  void DocumentLoaded();

  v8::Local<v8::Object> GetV8ScriptableObjectForPluginFrame(
      v8::Isolate* isolate);

  blink::WebFrame* web_frame() { return web_frame_; }
  void set_web_frame(blink::WebFrame* web_frame) { web_frame_ = web_frame; }

  // Post a JavaScript message to the guest.
  void PostMessage(v8::Isolate* isolate, v8::Local<v8::Value> message);

 private:
  // WebAssociatedURLLoaderClient overrides.
  void DidReceiveData(const char* data, int data_length) override;
  void DidFinishLoading(double finish_time) override;

  MimeHandlerViewManager* const manager_;
  int32_t unique_id_ = -1;
  std::unique_ptr<blink::WebAssociatedURLLoader> loader_;
  std::string view_id_;
  blink::WebFrame* web_frame_;

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
