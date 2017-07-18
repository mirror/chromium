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

 private:
  // WebAssociatedURLLoaderClient overrides.
  void DidReceiveData(const char* data, int data_length) override;
  void DidFinishLoading(double finish_time) override;

  MimeHandlerViewManager* const manager_;
  int32_t unique_id_ = -1;
  std::unique_ptr<blink::WebAssociatedURLLoader> loader_;
  std::string view_id_;

  DISALLOW_COPY_AND_ASSIGN(MimeHandlerViewFrameController);
};
}  // namespace extensions

#endif  // EXTENSIONS_RENDERER_MIME_HANDLER_VIEW_MIME_HANDLER_VIEW_FRAME_CONTROLLER_H_
