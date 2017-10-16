// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/html/canvas/CanvasDrawListener.h"

#include <memory>
#include "platform/graphics/StaticBitmapImage.h"
#include "public/platform/WebGraphicsContext3DProvider.h"

namespace blink {

CanvasDrawListener::~CanvasDrawListener() {}

void CanvasDrawListener::SendNewFrame(RefPtr<StaticBitmapImage> image) {
  handler_->SendNewFrame(image->PaintImageForCurrentFrame().GetSkImage(),
                         image->ContextProvider()->ContextGL());
}

bool CanvasDrawListener::NeedsNewFrame() const {
  return frame_capture_requested_ && handler_->NeedsNewFrame();
}

void CanvasDrawListener::RequestFrame() {
  frame_capture_requested_ = true;
}

CanvasDrawListener::CanvasDrawListener(
    std::unique_ptr<WebCanvasCaptureHandler> handler)
    : frame_capture_requested_(true), handler_(std::move(handler)) {}

}  // namespace blink
