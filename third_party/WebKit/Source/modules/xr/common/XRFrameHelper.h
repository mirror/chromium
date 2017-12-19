// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef XRFrameHelper_h
#define XRFrameHelper_h

#include "device/vr/vr_service.mojom-blink.h"

namespace blink {

class WebGLRenderingContextBase;
class Image;

class XRFrameHelper {
 public:
  static device::mojom::blink::VRRequestPresentOptionsPtr
  GetRequestPresentOptions(WebGLRenderingContextBase*);

  static scoped_refptr<Image> CaptureImage(WebGLRenderingContextBase*);

 private:
  // Static methods only, prevent instantiation.
  XRFrameHelper();
};

}  // namespace blink

#endif  // XRFrameHelper_h
