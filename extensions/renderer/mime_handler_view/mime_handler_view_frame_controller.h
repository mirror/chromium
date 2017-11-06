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
#include "third_party/WebKit/public/web/WebElement.h"
#include "url/gurl.h"
#include "v8/include/v8.h"

namespace blink {
class WebAssociatedURLLoader;
class WebElement;
class WebFrame;
struct WebURLError;
}  // namespace blink

namespace extensions {
class MimeHandlerViewManager;

// This class manages a plugin frame (a frame which is a CotentFrame for a
// HTMLPlugInElement) in navigating to the handler extension for specific mime
// types. This involves managing resource loading for the extension and
// providing custom javascript API on the element. This class is owned by
// MimeHandlerViewManager.
class MimeHandlerViewFrameController
    : public blink::WebAssociatedURLLoaderClient {
 public:
  MimeHandlerViewFrameController(MimeHandlerViewManager* manager,
                                 const blink::WebElement& owner_elmenet,
                                 const GURL& resource_url);
  ~MimeHandlerViewFrameController() override;

  // WebAssociatedURLLoaderClient overrides.
  void DidReceiveData(const char* data, int data_length) override;
  void DidFinishLoading(double finish_time) override;
  void DidFail(const blink::WebURLError&) override;

  // These methods are called by MimeHandlerViewManager.
  // Starts a URL request to load the PDF resource. The response will be
  // intercepted by the browser as an stream and a GUID is sent down to the
  // load to uniquely identify this controller and its stream. The stream id and
  // the corresponding frame/proxy's routing ID are sent back to the browser to
  // start navigation to the final handler (extension).
  void RequestResource();

  // Called when the corresponding |owner_element_|'s source attribute has
  // changed. This would involve resetting the corresponding |loader_| and
  // canceling any ongoing loading for the current |resource_url_|. In case the
  // resource has already loaded (and perhaps the frame has finished navigation
  // to the external handler), the MimeHandlerViewManager sends a request to
  // the browser right before calling this method to first drop the current
  // StreamContainer associated with |stream_id_|. After the stream is removed,
  // the browser notifies the renderer and then a new request for the new
  // value of |resource_url| is created. When |stream_id_| is not set the
  // request to load the resource is sent immediately.
  void ReplaceResource(const GURL& resource_url);

  // The current stream has been aborted. At this stage, a new resource request
  // is created.
  void DidAbortStream();

  // Used to notify that the document has loaded inside the frame corresponding
  // to this controller. At this point, all the (queued up) posted messages can
  // be sent to the extension process.
  void DocumentLoaded();

  // Returns a wrapped scriptable object which provides the implementation of
  // a postMessage API for the corresponding owner element.
  v8::Local<v8::Object> GetV8ScriptableObject(v8::Isolate* isolate);

  // Post a JavaScript message to the guest.
  void PostMessage(v8::Isolate* isolate, v8::Local<v8::Value> message);

  // The plugin frame associated with this controller.
  blink::WebFrame* GetPluginFrame() const;

  const blink::WebElement& owner_element() const { return owner_element_; }
  const GURL& resource_url() const { return resource_url_; }
  const std::string& stream_id() const { return stream_id_; }

 private:
  MimeHandlerViewManager* const manager_;

  // Used to communicate with blink.
  const blink::WebElement owner_element_;

  // The URL of the PDF resource. This might change during the lifetime of the
  // controller.
  GURL resource_url_;

  // Used to request the PDF resource when <embed> is not the first in a
  // PluginDocument.
  std::unique_ptr<blink::WebAssociatedURLLoader> loader_;

  // |stream_id| is a GUID which will uniquely identify the StreamContainer on
  // the browser side.
  std::string stream_id_;

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
