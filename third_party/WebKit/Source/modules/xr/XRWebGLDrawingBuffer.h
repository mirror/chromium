// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef XRWebGLDrawingBuffer_h
#define XRWebGLDrawingBuffer_h

#include "gpu/command_buffer/common/mailbox_holder.h"
#include "modules/webgl/WebGL2RenderingContext.h"
#include "modules/webgl/WebGLRenderingContext.h"
#include "modules/xr/XRFrameProvider.h"
#include "modules/xr/XRLayer.h"
#include "modules/xr/XRWebGLLayerInit.h"
#include "platform/geometry/IntSize.h"
#include "platform/heap/Handle.h"

namespace blink {

class StaticBitmapImage;
class WebGLFramebuffer;

class XRWebGLDrawingBuffer final
    : public GarbageCollected<XRWebGLDrawingBuffer>,
      public TraceWrapperBase,
      public XRFrameProvider::ImageSource {
 public:
  static XRWebGLDrawingBuffer* Create(WebGLRenderingContextBase*,
                                      const IntSize&,
                                      bool want_alpha_channel,
                                      bool want_depth_buffer,
                                      bool want_stencil_buffer,
                                      bool want_antialiasing,
                                      bool want_multiview);

  WebGLRenderingContextBase* webgl_context() const { return webgl_context_; }

  WebGLFramebuffer* framebuffer() const { return framebuffer_; }
  const IntSize& size() const { return size_; }

  bool antialias() const { return anti_aliasing_mode_ != kNone; }
  bool depth() const { return depth_; }
  bool stencil() const { return stencil_; }
  bool alpha() const { return alpha_; }
  bool multiview() const { return multiview_; }

  void Resize(const IntSize&);

  // Activate marks the framebuffer as complete and clears it's changed bit.
  void Activate();
  // Finish marks the framebuffer as incomplete and returns whether it was
  // changed while active.
  bool Finish();

  scoped_refptr<StaticBitmapImage> TransferToStaticBitmapImage() override;

  virtual void Trace(blink::Visitor*);
  virtual void TraceWrappers(const ScriptWrappableVisitor*) const;

 private:
  XRWebGLDrawingBuffer(WebGLRenderingContextBase*,
                       bool discard_framebuffer_supported,
                       bool want_alpha_channel,
                       bool want_depth_buffer,
                       bool want_stencil_buffer,
                       bool multiview_supported);

  gpu::gles2::GLES2Interface* gl_context() {
    return webgl_context_->ContextGL();
  }

  bool Initialize(const IntSize&, bool use_multisampling, bool use_multiview);

  GLuint CreateColorBuffer();

  bool WantExplicitResolve() const;
  void SwapColorBuffers();

  TraceWrapperMember<WebGLRenderingContextBase> webgl_context_;

  Member<WebGLFramebuffer> framebuffer_;
  GLuint resolved_framebuffer_ = 0;
  GLuint multisample_renderbuffer_ = 0;
  GLuint back_color_buffer_ = 0;
  GLuint front_color_buffer_ = 0;
  GLuint depth_stencil_buffer_ = 0;
  IntSize size_;

  bool discard_framebuffer_supported_;
  bool depth_;
  bool stencil_;
  bool alpha_;
  bool multiview_;

  enum WebGLVersion {
    kWebGL1,
    kWebGL2,
  };

  WebGLVersion webgl_version_ = kWebGL1;

  enum AntialiasingMode {
    kNone,
    kMSAAImplicitResolve,
    kMSAAExplicitResolve,
    kScreenSpaceAntialiasing,
  };

  AntialiasingMode anti_aliasing_mode_ = kNone;

  bool storage_texture_supported_ = false;
  int sample_count_ = 0;
};

}  // namespace blink

#endif  // XRWebGLDrawingBuffer_h
