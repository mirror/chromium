// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/renderer/mime_handler_view/mime_handler_view_frame_controller.h"

#include "base/threading/thread_task_runner_handle.h"
#include "content/public/renderer/render_frame.h"
#include "extensions/renderer/mime_handler_view/mime_handler_view_manager.h"
#include "third_party/WebKit/public/platform/WebURLRequest.h"
#include "third_party/WebKit/public/web/WebAssociatedURLLoader.h"
#include "third_party/WebKit/public/web/WebAssociatedURLLoaderOptions.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "url/gurl.h"

namespace extensions {
MimeHandlerViewFrameController::MimeHandlerViewFrameController(
    MimeHandlerViewManager* manager)
    : manager_(manager) {
  DCHECK(manager);
}

MimeHandlerViewFrameController::~MimeHandlerViewFrameController() {}

void MimeHandlerViewFrameController::RequestResource(
    const std::string& resource_url) {
  view_id_.clear();

  blink::WebLocalFrame* frame = manager_->render_frame()->GetWebFrame();
  blink::WebAssociatedURLLoaderOptions options;
  DCHECK(!loader_);
  loader_.reset(frame->CreateAssociatedURLLoader(options));

  GURL url(resource_url);
  blink::WebURLRequest request(url);
  request.SetRequestContext(blink::WebURLRequest::kRequestContextObject);
  loader_->LoadAsynchronously(request, this);
}

void MimeHandlerViewFrameController::SetId(int32_t unique_id) {
  DCHECK_EQ(unique_id_, -1);
  unique_id_ = unique_id;
}

void MimeHandlerViewFrameController::DocumentLoaded() {
  if (loader_)
    loader_.reset(nullptr);
}

void MimeHandlerViewFrameController::DidReceiveData(const char* data,
                                                    int data_length) {
  view_id_ += data;
}

void MimeHandlerViewFrameController::DidFinishLoading(double finish_time) {
  manager_->SetViewId(unique_id_, view_id_);
}

}  // namespace extensions
