// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/vr/latest/VRWebGLLayer.h"

#include "core/imagebitmap/ImageBitmap.h"
#include "modules/vr/latest/VRDevice.h"
#include "modules/vr/latest/VRFrameProvider.h"
#include "modules/vr/latest/VRPresentationContext.h"
#include "modules/vr/latest/VRSession.h"
#include "modules/vr/latest/VRView.h"
#include "modules/vr/latest/VRViewport.h"
#include "modules/webgl/WebGLFramebuffer.h"

namespace blink {

VRWebGLLayer* VRWebGLLayer::Create(
    VRSession* session,
    const WebGLRenderingContextOrWebGL2RenderingContext& context,
    const VRWebGLLayerInit& initializer) {
  WebGLRenderingContextBase* webgl_context;
  if (context.isWebGL2RenderingContext()) {
    webgl_context = context.getAsWebGL2RenderingContext();
  } else {
    webgl_context = context.getAsWebGLRenderingContext();
  }

  bool want_antialiasing = initializer.antialias();
  bool want_depth_buffer = initializer.depth();
  bool want_stencil_buffer = initializer.stencil();
  bool want_alpha_channel = initializer.alpha();
  bool want_multiview = initializer.multiview();

  double framebuffer_scale = session->IdealFramebufferScale();

  if (initializer.hasFramebufferScaleFactor() &&
      initializer.framebufferScaleFactor() != 0.0) {
    framebuffer_scale =
        std::min(1.2, std::max(0.2, initializer.framebufferScaleFactor()));
  }

  IntSize desired_size(session->IdealFramebufferWidth() * framebuffer_scale,
                       session->IdealFramebufferHeight() * framebuffer_scale);

  VRWebGLDrawingBuffer* drawing_buffer = VRWebGLDrawingBuffer::Create(
      webgl_context, desired_size, want_alpha_channel, want_depth_buffer,
      want_stencil_buffer, want_antialiasing, want_multiview);

  if (!drawing_buffer) {
    return nullptr;
  }

  return new VRWebGLLayer(session, drawing_buffer, framebuffer_scale);
}

VRWebGLLayer::VRWebGLLayer(VRSession* session,
                           VRWebGLDrawingBuffer* drawing_buffer,
                           double framebuffer_scale)
    : VRLayer(session, kVRWebGLLayerType),
      drawing_buffer_(drawing_buffer),
      framebuffer_scale_(framebuffer_scale) {
  DCHECK(drawing_buffer);
  UpdateViewports();
}

void VRWebGLLayer::getVRWebGLRenderingContext(
    WebGLRenderingContextOrWebGL2RenderingContext& result) const {
  WebGLRenderingContextBase* webgl_context = drawing_buffer_->webgl_context();
  if (webgl_context->Version() == 2) {
    result.setWebGL2RenderingContext(
        static_cast<WebGL2RenderingContext*>(webgl_context));
  } else {
    result.setWebGLRenderingContext(
        static_cast<WebGLRenderingContext*>(webgl_context));
  }
}

void VRWebGLLayer::requestViewportScaling(double scale_factor) {
  if (!session()->exclusive()) {
    // TODO(bajones): For the moment we're jsut going to ignore viewport changes
    // in non-exclusive mode. This is legal, but probably not what developers
    // would like to see. Look into making viewport scale apply properly.
    scale_factor = 1.0;
  } else {
    scale_factor = std::min(1.0, std::max(0.2, scale_factor));
  }

  if (viewport_scale_ != scale_factor) {
    viewport_scale_ = scale_factor;
    viewports_dirty_ = true;
  }
}

VRViewport* VRWebGLLayer::GetViewport(VRView::Eye eye) {
  if (eye == VRView::kEyeLeft)
    return left_viewport_;

  return right_viewport_;
}

void VRWebGLLayer::OnFrameStart() {
  if (viewports_dirty_)
    UpdateViewports();
}

void VRWebGLLayer::OnFrameEnd() {
  // Submit the frame to the VR compositor.

  if (session()->exclusive()) {
    // TODO: There's definitely a more elegant way to do this.
    gpu::MailboxHolder mailbox_holder = drawing_buffer_->GetMailbox();
    session()->device()->frameProvider()->SubmitFrame(mailbox_holder);
  } else if (session()->outputContext()) {
    ImageBitmap* image_bitmap =
        ImageBitmap::Create(drawing_buffer_->TransferToStaticBitmapImage());
    session()->outputContext()->transferFromImageBitmap(image_bitmap);
  }

  // TODO: Needed?
  // drawing_buffer_->MarkCompositedAndClear();
}

void VRWebGLLayer::OnResize() {
  IntSize desired_size(
      session()->IdealFramebufferWidth() * framebuffer_scale_,
      session()->IdealFramebufferHeight() * framebuffer_scale_);
  drawing_buffer_->Resize(desired_size);
  viewports_dirty_ = true;
}

void VRWebGLLayer::UpdateViewports() {
  long framebuffer_width = framebufferWidth();
  long framebuffer_height = framebufferHeight();

  if (session()->exclusive()) {
    left_viewport_ =
        new VRViewport(0, 0, framebuffer_width * 0.5 * viewport_scale_,
                       framebuffer_height * viewport_scale_);
    right_viewport_ =
        new VRViewport(framebuffer_width * 0.5 * viewport_scale_, 0,
                       framebuffer_width * 0.5 * viewport_scale_,
                       framebuffer_height * viewport_scale_);

    session()->device()->frameProvider()->UpdateWebGLLayerViewports(this);
  } else {
    left_viewport_ = new VRViewport(0, 0, framebuffer_width * viewport_scale_,
                                    framebuffer_height * viewport_scale_);
  }

  viewports_dirty_ = false;
}

DEFINE_TRACE(VRWebGLLayer) {
  visitor->Trace(left_viewport_);
  visitor->Trace(right_viewport_);
  visitor->Trace(drawing_buffer_);
  VRLayer::Trace(visitor);
}

}  // namespace blink
