// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/vr/latest/VRWebGLLayer.h"

#include "modules/vr/latest/VRSession.h"
#include "modules/webgl/WebGLFramebuffer.h"

namespace blink {

VRWebGLLayer::VRWebGLLayer(VRSession* session,
                           WebGLRenderingContextBase* context,
                           const VRWebGLLayerInit& initializer)
    : session_(session), context_(context) {}

void VRWebGLLayer::getVRWebGLRenderingContext(
    WebGLRenderingContextOrWebGL2RenderingContext& result) const {
  if (context()->Version() == 2) {
    result.setWebGL2RenderingContext(
        static_cast<WebGL2RenderingContext*>(context()));
  } else {
    result.setWebGLRenderingContext(
        static_cast<WebGLRenderingContext*>(context()));
  }
}

void VRWebGLLayer::requestViewportScaling(double viewportScaleFactor) {
  // TODO(bajones): Do something here (though it IS valid to ignore!)
}

DEFINE_TRACE(VRWebGLLayer) {
  visitor->Trace(session_);
  visitor->Trace(context_);
  visitor->Trace(framebuffer_);
  VRLayer::Trace(visitor);
}

}  // namespace blink
