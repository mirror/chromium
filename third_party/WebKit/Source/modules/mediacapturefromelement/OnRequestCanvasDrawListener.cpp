// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/mediacapturefromelement/OnRequestCanvasDrawListener.h"

#include <memory>
#include "platform/graphics/StaticBitmapImage.h"

namespace blink {

OnRequestCanvasDrawListener::OnRequestCanvasDrawListener(
    std::unique_ptr<WebCanvasCaptureHandler> handler)
    : CanvasDrawListener(std::move(handler)) {}

OnRequestCanvasDrawListener::~OnRequestCanvasDrawListener() {}

// static
OnRequestCanvasDrawListener* OnRequestCanvasDrawListener::Create(
    std::unique_ptr<WebCanvasCaptureHandler> handler) {
  return new OnRequestCanvasDrawListener(std::move(handler));
}

void OnRequestCanvasDrawListener::SendNewFrame(
    RefPtr<StaticBitmapImage> image) {
  frame_capture_requested_ = false;
  CanvasDrawListener::SendNewFrame(image);
}

}  // namespace blink
