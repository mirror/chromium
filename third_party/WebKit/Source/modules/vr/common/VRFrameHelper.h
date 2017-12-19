// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef VRFrameHelper_h
#define VRFrameHelper_h

#include "device/vr/vr_service.mojom-blink.h"

namespace blink {

class WebGLRenderingContextBase;
class Image;

class VRFrameHelper {
 public:
  static device::mojom::blink::VRRequestPresentOptionsPtr
  GetRequestPresentOptions(WebGLRenderingContextBase*);

  static scoped_refptr<Image> CaptureImage(WebGLRenderingContextBase*);

 private:
  // Static methods only, prevent instantiation.
  VRFrameHelper();
};

}  // namespace blink

#endif  // VRFrameHelper_h
