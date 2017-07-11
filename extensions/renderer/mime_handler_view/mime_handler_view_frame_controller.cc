// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/renderer/mime_handler_view/mime_handler_view_frame_controller.h"

#include "base/threading/thread_task_runner_handle.h"
#include "content/public/renderer/render_frame.h"
#include "third_party/WebKit/public/platform/WebURLRequest.h"
#include "third_party/WebKit/public/web/WebAssociatedURLLoader.h"
#include "third_party/WebKit/public/web/WebAssociatedURLLoaderOptions.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "url/gurl.h"

namespace extensions {
MimeHandlerViewFrameController::MimeHandlerViewFrameController(int32_t id)
    : unique_id_(id) {}

MimeHandlerViewFrameController::~MimeHandlerViewFrameController() {}

void MimeHandlerViewFrameController::RequestResource(
    const std::string& resource_url) {
  content::RenderFrame* render_frame =
      content::RenderFrame::FromRoutingID(unique_id_);
  if (!render_frame)
    return;

  blink::WebLocalFrame* frame = render_frame->GetWebFrame();
  blink::WebAssociatedURLLoaderOptions options;
  DCHECK(!loader_);
  loader_.reset(frame->CreateAssociatedURLLoader(options));

  GURL url(resource_url);
  blink::WebURLRequest request(url);
  request.SetRequestContext(blink::WebURLRequest::kRequestContextObject);
  loader_->LoadAsynchronously(request, this);
}

void MimeHandlerViewFrameController::DocumentLoaded() {
  if (loader_)
    loader_.reset(nullptr);
}

}  // namespace extensions
