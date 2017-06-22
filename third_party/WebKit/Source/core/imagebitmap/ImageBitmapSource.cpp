// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/imagebitmap/ImageBitmapSource.h"

#include "core/imagebitmap/ImageBitmap.h"
#include "core/imagebitmap/ImageBitmapOptions.h"

namespace blink {

ScriptPromise ImageBitmapSource::CreateImageBitmap(
    ScriptState* script_state,
    EventTarget& event_target,
    Optional<IntRect> crop_rect,
    const ImageBitmapOptions& options,
    ExceptionState& exception_state) {
  return ScriptPromise();
}

}  // namespace blink
