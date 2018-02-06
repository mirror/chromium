// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "modules/canvas/htmlcanvas/CanvasContextCreationAttributesHelpers.h"

#include "core/html/canvas/CanvasContextCreationAttributes.h"
#include "modules/canvas/htmlcanvas/CanvasContextCreationAttributesModule.h"
#include "third_party/WebKit/Source/modules/xr/XRDevice.h"

namespace blink {

CanvasContextCreationAttributes ToCanvasContextCreationAttributes(
    const CanvasContextCreationAttributesModule& attrs) {
  CanvasContextCreationAttributes result;
  result.setAlpha(attrs.alpha());
  result.setAntialias(attrs.antialias());
  result.setColorSpace(attrs.colorSpace());
  result.setDepth(attrs.depth());
  result.setFailIfMajorPerformanceCaveat(attrs.failIfMajorPerformanceCaveat());
  result.setLowLatency(attrs.lowLatency());
  result.setPixelFormat(attrs.pixelFormat());
  result.setPremultipliedAlpha(attrs.premultipliedAlpha());
  result.setPreserveDrawingBuffer(attrs.preserveDrawingBuffer());
  result.setStencil(attrs.stencil());
  result.setCompatibleXRDevice(attrs.compatibleXRDevice());
  return result;
}

}  // namespace blink
