// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef VRWebGLLayer_h
#define VRWebGLLayer_h

#include "bindings/modules/v8/WebGLRenderingContextOrWebGL2RenderingContext.h"
#include "modules/vr/latest/VRLayer.h"
#include "modules/vr/latest/VRWebGLLayerInit.h"
#include "modules/webgl/WebGL2RenderingContext.h"
#include "modules/webgl/WebGLRenderingContext.h"

namespace blink {

class WebGLFramebuffer;
class VRSession;

class VRWebGLLayer final : public VRLayer {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static VRWebGLLayer* Create(
      VRSession* session,
      const WebGLRenderingContextOrWebGL2RenderingContext& context,
      const VRWebGLLayerInit& initializer) {
    if (context.isWebGL2RenderingContext()) {
      return new VRWebGLLayer(session, context.getAsWebGL2RenderingContext(),
                              initializer);
    } else {
      return new VRWebGLLayer(session, context.getAsWebGLRenderingContext(),
                              initializer);
    }
  }

  VRSession* session() const { return session_; }
  WebGLRenderingContextBase* context() const { return context_; }
  void getVRWebGLRenderingContext(
      WebGLRenderingContextOrWebGL2RenderingContext&) const;

  WebGLFramebuffer* framebuffer() const { return framebuffer_; }
  long framebufferWidth() const { return framebuffer_width_; }
  long framebufferHeight() const { return framebuffer_height_; }

  bool antialias() const { return antialias_; }
  bool depth() const { return depth_; }
  bool stencil() const { return stencil_; }
  bool alpha() const { return alpha_; }
  bool multiview() const { return multiview_; }

  void requestViewportScaling(double viewportScaleFactor);

  DECLARE_VIRTUAL_TRACE();

 private:
  VRWebGLLayer(VRSession*,
               WebGLRenderingContextBase*,
               const VRWebGLLayerInit& initializer);

  Member<VRSession> session_;
  Member<WebGLRenderingContextBase> context_;

  Member<WebGLFramebuffer> framebuffer_;
  long framebuffer_width_;
  long framebuffer_height_;

  bool antialias_;
  bool depth_;
  bool stencil_;
  bool alpha_;
  bool multiview_;
};

}  // namespace blink

#endif  // VRWebGLLayer_h
