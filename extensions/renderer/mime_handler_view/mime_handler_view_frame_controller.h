// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_RENDERER_MIME_HANDLER_VIEW_MIME_HANDLER_VIEW_FRAME_CONTROLLER_H_
#define EXTENSIONS_RENDERER_MIME_HANDLER_VIEW_MIME_HANDLER_VIEW_FRAME_CONTROLLER_H_

#include "third_party/WebKit/public/platform/WebURL.h"
#include "third_party/WebKit/public/web/WebAssociatedURLLoaderClient.h"
#include "url/gurl.h"

namespace blink {
class WebAssociatedURLLoader;
}

namespace extensions {

class MimeHandlerViewFrameController
    : public blink::WebAssociatedURLLoaderClient {
 public:
  explicit MimeHandlerViewFrameController(int32_t unique_id);
  ~MimeHandlerViewFrameController() override;

  void RequestResource(const std::string& resource_url);
  void DocumentLoaded();

 private:
  const int32_t unique_id_;
  std::unique_ptr<blink::WebAssociatedURLLoader> loader_;

  DISALLOW_COPY_AND_ASSIGN(MimeHandlerViewFrameController);
};
}  // namespace extensions

#endif  // EXTENSIONS_RENDERER_MIME_HANDLER_VIEW_MIME_HANDLER_VIEW_FRAME_CONTROLLER_H_
