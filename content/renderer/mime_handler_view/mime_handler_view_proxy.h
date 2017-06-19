// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MIME_HANDLER_VIEW_MIME_HANDLER_VIEW_PROXY_
#define CONTENT_RENDERER_MIME_HANDLER_VIEW_MIME_HANDLER_VIEW_PROXY_

#include <unordered_map>

#include "base/macros.h"
#include "url/gurl.h"

namespace blink {
class WebFrame;
class WebString;
class WebURL;
}  // namespace blink

namespace v8 {
class Isolate;
template <typename T>
class Local;
class Object;
}  // namespace v8

namespace content {
class RenderFrameImpl;

class MimeHandlerViewProxy {
 public:
  MimeHandlerViewProxy(RenderFrameImpl* const render_frame_impl);
  ~MimeHandlerViewProxy();

  static MimeHandlerViewProxy* FromWebFrame(blink::WebFrame* frame);

  // When loading the given |url| with mime type determined by
  // |original_mime_type| is done by navigating a frame to a corresponding
  // extension, the value of |overriden_url| is overriden by a URL with a
  // 'chrome-extension' scheme. Otherwise the value of |overriden_url| is
  // untouched.
  void OverrrideURLForPDFEmbed(const blink::WebURL& url,
                               const blink::WebString& original_mime_type,
                               blink::WebURL* overriden_url);

  // If the
  void MaybeRequestPDFResource(RenderFrameImpl* navigating_frame,
                               const GURL& complete_url);

  v8::Local<v8::Object> GetV8ScriptableObjectForPluginFrame(
      v8::Isolate* isolate,
      blink::WebFrame* frame);

  void FrameSwappedWithProxy(int32_t frame_routing_id,
                             int32_t proxy_routing_id);

  void FrameDetached(int32_t routing_id);

 private:
  int32_t GetOriginalFrameRoutingIDFromProxy(int proxy_id);

  RenderFrameImpl* const render_frame_impl_;
  std::unordered_map<int32_t, int32_t> routing_id_map_;

  DISALLOW_COPY_AND_ASSIGN(MimeHandlerViewProxy);
};

}  // namespace content
#endif  // CONTENT_RENDERER_MIME_HANDLER_VIEW_MIME_HANDLER_VIEW_PROXY_
