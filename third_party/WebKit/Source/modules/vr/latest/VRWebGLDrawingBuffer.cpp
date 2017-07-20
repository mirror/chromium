// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/vr/latest/VRWebGLDrawingBuffer.h"

#include "gpu/command_buffer/client/gles2_interface.h"
#include "modules/vr/latest/VRSession.h"
#include "modules/vr/latest/VRView.h"
#include "modules/vr/latest/VRViewport.h"
#include "modules/webgl/WebGLFramebuffer.h"
#include "modules/webgl/WebGLRenderbuffer.h"
#include "modules/webgl/WebGLTexture.h"
#include "platform/graphics/gpu/Extensions3DUtil.h"

namespace blink {

VRWebGLDrawingBuffer* VRWebGLDrawingBuffer::Create(
    WebGLRenderingContextBase* webgl_context,
    const IntSize& size,
    bool want_alpha_channel,
    bool want_depth_buffer,
    bool want_stencil_buffer,
    bool want_antialiasing,
    bool want_multiview) {
  DCHECK(webgl_context);

  std::unique_ptr<Extensions3DUtil> extensions_util =
      Extensions3DUtil::Create(webgl_context->ContextGL());
  if (!extensions_util->IsValid()) {
    // This might be the first time we notice that the GL context is lost.
    return nullptr;
  }

  DCHECK(extensions_util->SupportsExtension("GL_OES_packed_depth_stencil"));
  extensions_util->EnsureExtensionEnabled("GL_OES_packed_depth_stencil");
  bool multisample_supported =
      want_antialiasing &&
      (extensions_util->SupportsExtension(
           "GL_CHROMIUM_framebuffer_multisample") ||
       extensions_util->SupportsExtension(
           "GL_EXT_multisampled_render_to_texture")) &&
      extensions_util->SupportsExtension("GL_OES_rgb8_rgba8");
  if (multisample_supported) {
    extensions_util->EnsureExtensionEnabled("GL_OES_rgb8_rgba8");
    if (extensions_util->SupportsExtension(
            "GL_CHROMIUM_framebuffer_multisample")) {
      extensions_util->EnsureExtensionEnabled(
          "GL_CHROMIUM_framebuffer_multisample");
    } else {
      extensions_util->EnsureExtensionEnabled(
          "GL_EXT_multisampled_render_to_texture");
    }
  }
  bool discard_framebuffer_supported =
      extensions_util->SupportsExtension("GL_EXT_discard_framebuffer");
  if (discard_framebuffer_supported)
    extensions_util->EnsureExtensionEnabled("GL_EXT_discard_framebuffer");

  // TODO(bajones): Support multiview
  bool multiview_supported = false;

  VRWebGLDrawingBuffer* drawing_buffer = new VRWebGLDrawingBuffer(
      webgl_context, size, discard_framebuffer_supported, want_alpha_channel,
      want_depth_buffer, want_stencil_buffer, multiview_supported);
  if (!drawing_buffer->Initialize(multisample_supported, multiview_supported)) {
    // drawing_buffer->BeginDestruction();
    return nullptr;
  }
  return drawing_buffer;
}

VRWebGLDrawingBuffer::VRWebGLDrawingBuffer(
    WebGLRenderingContextBase* webgl_context,
    const IntSize& size,
    bool discard_framebuffer_supported,
    bool want_alpha_channel,
    bool want_depth_buffer,
    bool want_stencil_buffer,
    bool multiview_supported)
    : webgl_context_(webgl_context),
      size_(size),
      antialias_(false),
      depth_(want_depth_buffer),
      stencil_(want_stencil_buffer),
      alpha_(want_alpha_channel),
      multiview_(false) {
  contents_changed_ = true;  // TODO: Obviously something better than this.
}

bool VRWebGLDrawingBuffer::Initialize(bool use_multisampling,
                                      bool use_multiview) {
  gpu::gles2::GLES2Interface* gl = gl_context();

  // TODO(bajones): Everything about this is ugly. Find a more elegant way to
  // create a WebGL-compatible framebuffer and manage it's state.

  // Create a WebGL Framebuffer
  // TODO(bajones): The framebuffer should be designated as opaque so that the
  // internal attachements can't be retrieved.
  framebuffer_ = WebGLFramebuffer::Create(webgl_context_);
  GLuint framebuffer_id = framebuffer_->Object();

  gl->BindFramebuffer(GL_FRAMEBUFFER, framebuffer_id);

  if (depth_ || stencil_) {
    depth_stencil_buffer_ = WebGLRenderbuffer::Create(webgl_context_);
    GLuint depth_stencil_id = depth_stencil_buffer_->Object();

    gl->BindRenderbuffer(GL_RENDERBUFFER, depth_stencil_id);
    gl->RenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8_OES,
                            size_.Width(), size_.Height());

    // Calls glFramebufferRenderbuffer internally
    framebuffer_->SetAttachmentForBoundFramebuffer(
        GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, depth_stencil_buffer_);

    depth_stencil_buffer_->SetInternalFormat(GL_DEPTH24_STENCIL8_OES);
    depth_stencil_buffer_->SetSize(size_.Width(), size_.Height());

    webgl_context_->DrawingBufferClientRestoreRenderbufferBinding();
  }

  back_color_buffer_ = CreateColorBuffer();
  front_color_buffer_ = CreateColorBuffer();

  // Calls glFramebufferTexture2D internally
  framebuffer_->SetAttachmentForBoundFramebuffer(
      GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, back_color_buffer_,
      0, 0);

  webgl_context_->DrawingBufferClientRestoreFramebufferBinding();

  /*gpu::gles2::GLES2Interface* gl = gl_context();

  ScopedStateRestorer scoped_state_restorer(this);

  if (gl->GetGraphicsResetStatusKHR() != GL_NO_ERROR) {
    // Need to try to restore the context again later.
    return false;
  }

  gl->GetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size_);

  int max_sample_count = 0;
  anti_aliasing_mode_ = kNone;
  if (use_multisampling) {
    gl->GetIntegerv(GL_MAX_SAMPLES_ANGLE, &max_sample_count);
    anti_aliasing_mode_ = kMSAAExplicitResolve;
    if (extensions_util_->SupportsExtension(
            "GL_EXT_multisampled_render_to_texture")) {
      anti_aliasing_mode_ = kMSAAImplicitResolve;
    } else if (extensions_util_->SupportsExtension(
                   "GL_CHROMIUM_screen_space_antialiasing")) {
      anti_aliasing_mode_ = kScreenSpaceAntialiasing;
    }
  }
  // TODO(dshwang): Enable storage textures on all platforms. crbug.com/557848
  // The Linux ATI bot fails
  // WebglConformance.conformance_textures_misc_tex_image_webgl, so use storage
  // textures only if ScreenSpaceAntialiasing is enabled, because
  // ScreenSpaceAntialiasing is much faster with storage textures.
  storage_texture_supported_ =
      (web_gl_version_ > kWebGL1 ||
       extensions_util_->SupportsExtension("GL_EXT_texture_storage")) &&
      anti_aliasing_mode_ == kScreenSpaceAntialiasing;
  sample_count_ = std::min(4, max_sample_count);

  state_restorer_->SetFramebufferBindingDirty();
  gl->GenFramebuffers(1, &fbo_);
  gl->BindFramebuffer(GL_FRAMEBUFFER, fbo_);
  if (WantExplicitResolve()) {
    gl->GenFramebuffers(1, &multisample_fbo_);
    gl->BindFramebuffer(GL_FRAMEBUFFER, multisample_fbo_);
    gl->GenRenderbuffers(1, &multisample_renderbuffer_);
  }
  if (!ResizeFramebufferInternal(size))
    return false;

  if (depth_stencil_buffer_) {
    DCHECK(WantDepthOrStencil());
    has_implicit_stencil_buffer_ = !want_stencil_;
  }

  if (gl->GetGraphicsResetStatusKHR() != GL_NO_ERROR) {
    // It's possible that the drawing buffer allocation provokes a context loss,
    // so check again just in case. http://crbug.com/512302
    return false;
  }*/

  return true;
}

WebGLTexture* VRWebGLDrawingBuffer::CreateColorBuffer() {
  gpu::gles2::GLES2Interface* gl = gl_context();

  WebGLTexture* color_buffer = WebGLTexture::Create(webgl_context_);

  GLuint texture_id = color_buffer->Object();
  gl->BindTexture(GL_TEXTURE_2D, texture_id);
  gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  GLenum gl_format = alpha_ ? GL_RGBA : GL_RGB;
  gl->TexImage2D(GL_TEXTURE_2D, 0, gl_format, size_.Width(), size_.Height(), 0,
                 gl_format, GL_UNSIGNED_BYTE, 0);

  webgl_context_->DrawingBufferClientRestoreTexture2DBinding();

  return color_buffer;
}

gpu::MailboxHolder VRWebGLDrawingBuffer::GetBackBufferMailbox() {
  gpu::gles2::GLES2Interface* gl = gl_context();

  if (contents_changed_) {
    // ResolveIfNeeded();
    gl->Flush();
  }

  // Contexts may be in a different share group. We must transfer the texture
  // through a mailbox first.
  GLenum target = GL_TEXTURE_2D;
  gpu::Mailbox mailbox;
  gpu::SyncToken sync_token;

  gl->GenMailboxCHROMIUM(mailbox.name);
  gl->ProduceTextureDirectCHROMIUM(back_color_buffer_->Object(), target,
                                   mailbox.name);
  const GLuint64 fence_sync = gl->InsertFenceSyncCHROMIUM();
  gl->Flush();
  gl->GenSyncTokenCHROMIUM(fence_sync, sync_token.GetData());

  // Swap buffers
  WebGLTexture* tmp = back_color_buffer_;
  back_color_buffer_ = front_color_buffer_;
  front_color_buffer_ = tmp;

  gl->BindFramebuffer(GL_FRAMEBUFFER, framebuffer_->Object());
  framebuffer_->SetAttachmentForBoundFramebuffer(
      GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, back_color_buffer_,
      0, 0);
  webgl_context_->DrawingBufferClientRestoreFramebufferBinding();

  if (!sync_token.HasData()) {
    // This should only happen if the context has been lost.
    return gpu::MailboxHolder();
  }

  return gpu::MailboxHolder(mailbox, sync_token, target);
}

DEFINE_TRACE(VRWebGLDrawingBuffer) {
  visitor->Trace(webgl_context_);
  visitor->Trace(framebuffer_);
  visitor->Trace(back_color_buffer_);
  visitor->Trace(front_color_buffer_);
  visitor->Trace(depth_stencil_buffer_);
}

}  // namespace blink
