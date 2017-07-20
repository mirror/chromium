// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef VRWebGLDrawingBuffer_h
#define VRWebGLDrawingBuffer_h

#include "gpu/command_buffer/common/mailbox_holder.h"
#include "modules/vr/latest/VRLayer.h"
#include "modules/vr/latest/VRWebGLLayerInit.h"
#include "modules/webgl/WebGL2RenderingContext.h"
#include "modules/webgl/WebGLRenderingContext.h"
#include "platform/geometry/IntSize.h"
#include "platform/heap/Handle.h"

namespace blink {

class WebGLFramebuffer;
class WebGLRenderbuffer;
class WebGLTexture;

class VRWebGLDrawingBuffer final
    : public GarbageCollected<VRWebGLDrawingBuffer> {
 public:
  static VRWebGLDrawingBuffer* Create(WebGLRenderingContextBase*,
                                      const IntSize&,
                                      bool want_alpha_channel,
                                      bool want_depth_buffer,
                                      bool want_stencil_buffer,
                                      bool want_antialiasing,
                                      bool want_multiview);

  WebGLRenderingContextBase* webgl_context() const { return webgl_context_; }

  WebGLFramebuffer* framebuffer() const { return framebuffer_; }
  const IntSize& size() const { return size_; }

  bool antialias() const { return antialias_; }
  bool depth() const { return depth_; }
  bool stencil() const { return stencil_; }
  bool alpha() const { return alpha_; }
  bool multiview() const { return multiview_; }

  gpu::MailboxHolder GetBackBufferMailbox();

  DECLARE_TRACE();

 private:
  VRWebGLDrawingBuffer(WebGLRenderingContextBase*,
                       const IntSize&,
                       bool discard_framebuffer_supported,
                       bool want_alpha_channel,
                       bool want_depth_buffer,
                       bool want_stencil_buffer,
                       bool multiview_supported);

  gpu::gles2::GLES2Interface* gl_context() {
    return webgl_context_->ContextGL();
  }

  bool Initialize(bool use_multisampling, bool use_multiview);

  WebGLTexture* CreateColorBuffer();

  Member<WebGLRenderingContextBase> webgl_context_;

  Member<WebGLFramebuffer> framebuffer_;
  Member<WebGLTexture> back_color_buffer_;
  Member<WebGLTexture> front_color_buffer_;
  Member<WebGLRenderbuffer> depth_stencil_buffer_;
  const IntSize size_;

  bool antialias_;
  bool depth_;
  bool stencil_;
  bool alpha_;
  bool multiview_;

  bool contents_changed_;
};

}  // namespace blink

#endif  // VRWebGLDrawingBuffer_h
