// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/vr/common/VRFrameHelper.h"

#include "core/imagebitmap/ImageBitmap.h"
#include "modules/webgl/WebGLRenderingContextBase.h"
#include "platform/graphics/Image.h"

namespace blink {

// static
device::mojom::blink::VRRequestPresentOptionsPtr
VRFrameHelper::GetRequestPresentOptions(
    WebGLRenderingContextBase* rendering_context) {
  device::mojom::blink::VRRequestPresentOptionsPtr options =
      device::mojom::blink::VRRequestPresentOptions::New();
  options->preserveDrawingBuffer =
      rendering_context->CreationAttributes().preserveDrawingBuffer();
  return options;
}

scoped_refptr<Image> VRFrameHelper::CaptureImage(
    WebGLRenderingContextBase* rendering_context) {
  TRACE_EVENT_BEGIN0("gpu", "VRDisplay:GetStaticBitmapImage");
  scoped_refptr<Image> image_ref = rendering_context->GetStaticBitmapImage();
  TRACE_EVENT_END0("gpu", "VRDisplay::GetStaticBitmapImage");

  // Hardware-accelerated rendering should always be texture backed,
  // as implemented by AcceleratedStaticBitmapImage. Ensure this is
  // the case, don't attempt to render if using an unexpected drawing
  // path.
  if (!image_ref.get() || !image_ref->IsTextureBacked()) {
    TRACE_EVENT0("gpu", "VRDisplay::GetImage_SlowFallback");
    // We get a non-texture-backed image when running layout tests
    // on desktop builds. Add a slow fallback so that these continue
    // working.
    image_ref = rendering_context->GetImage(kPreferAcceleration,
                                            kSnapshotReasonCreateImageBitmap);
    if (!image_ref.get() || !image_ref->IsTextureBacked()) {
      NOTREACHED()
          << "WebVR requires hardware-accelerated rendering to texture";
      return nullptr;
    }
  }
  return image_ref;
}

}  // namespace blink
