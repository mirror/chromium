// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/html/canvas/CanvasRenderingContextHost.h"

#include "core/html/canvas/CanvasRenderingContext.h"
#include "platform/graphics/StaticBitmapImage.h"
#include "third_party/skia/include/core/SkSurface.h"

namespace blink {

CanvasRenderingContextHost::CanvasRenderingContextHost() {}

ScriptPromise CanvasRenderingContextHost::Commit(
    RefPtr<StaticBitmapImage> bitmap_image,
    const SkIRect& damage_rect,
    bool is_web_gl_software_rendering,
    ScriptState* script_state,
    ExceptionState& exception_state) {
  exception_state.ThrowDOMException(kInvalidStateError,
                                    "Commit() was called on a rendering "
                                    "context that was not created from an "
                                    "OffscreenCanvas.");
  return exception_state.Reject(script_state);
}

bool CanvasRenderingContextHost::IsPaintable() const {
  return (RenderingContext() && RenderingContext()->IsPaintable()) ||
         ImageBuffer::CanCreateImageBuffer(Size());
}

}  // namespace blink
