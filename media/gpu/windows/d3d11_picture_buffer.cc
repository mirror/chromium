// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/gpu/windows/d3d11_picture_buffer.h"

#include <d3d11.h>
#include <d3d11_1.h>
#include <dxva.h>
#include <windows.h>

#include "base/memory/ptr_util.h"
#include "base/trace_event/trace_event.h"
#include "base/win/scoped_comptr.h"
#include "gpu/command_buffer/service/mailbox_manager.h"
#include "gpu/command_buffer/service/texture_manager.h"
#include "media/gpu/gles2_decoder_helper.h"
//#include "media/gpu/h264_decoder.h"
//#include "media/gpu/h264_dpb.h"
#include "third_party/angle/include/EGL/egl.h"
#include "third_party/angle/include/EGL/eglext.h"
#include "ui/gfx/color_space.h"
#include "ui/gl/gl_angle_util_win.h"
#include "ui/gl/gl_bindings.h"
#include "ui/gl/gl_context.h"
#include "ui/gl/gl_image_dxgi.h"
#include "ui/gl/gl_surface_egl.h"
#include "ui/gl/scoped_binders.h"

namespace media {

namespace {

static bool MakeContextCurrent(gpu::GpuCommandBufferStub* stub) {
  return stub && stub->decoder()->MakeCurrent();
}

}  // namespace

D3D11PictureBuffer::D3D11PictureBuffer(GLenum target,
                                       gfx::Size size,
                                       size_t level)
    : target_(target), size_(size), level_(level) {}

D3D11PictureBuffer::~D3D11PictureBuffer() {}

bool D3D11PictureBuffer::Init(
    base::Callback<gpu::GpuCommandBufferStub*()> get_stub_cb,
    base::win::ScopedComPtr<ID3D11VideoDevice> video_device,
    base::win::ScopedComPtr<ID3D11Texture2D> texture,
    const GUID& decoder_guid,
    int textures_per_picture) {
  texture_ = texture;
  D3D11_VIDEO_DECODER_OUTPUT_VIEW_DESC view_desc = {};
  view_desc.DecodeProfile = decoder_guid;
  view_desc.ViewDimension = D3D11_VDOV_DIMENSION_TEXTURE2D;
  view_desc.Texture2D.ArraySlice = (UINT)level_;

  LOG(ERROR) << "AVDA: creating output view for " << level_;
  HRESULT hr = video_device->CreateVideoDecoderOutputView(
      texture.Get(), &view_desc, output_view_.GetAddressOf());

  CHECK(SUCCEEDED(hr));

  // Generate mailboxes and holders.
  std::vector<gpu::Mailbox> mailboxes;
  for (int texture_idx = 0; texture_idx < textures_per_picture; texture_idx++) {
    mailboxes.push_back(gpu::Mailbox::Generate());
    mailbox_holders_[texture_idx] = gpu::MailboxHolder(
        mailboxes[texture_idx], gpu::SyncToken(), GL_TEXTURE_EXTERNAL_OES);
  }

  // Start construction of the GpuResources.
  // We send the texture itself, since we assume that we're using the angle
  // device for decoding.  Sharing seems not to work very well.  Otherwise, we
  // would create the texture with KEYED_MUTEX and NTHANDLE, then send along
  // a handle that we get from |texture| as an IDXGIResource1.
  gpu_resources_ = base::MakeUnique<GpuResources>();
  if (!gpu_resources_->Init(std::move(get_stub_cb), level_,
                            std::move(mailboxes), target_, size_, texture,
                            textures_per_picture))
    return false;

  return true;
}

D3D11PictureBuffer::GpuResources::GpuResources() {}

D3D11PictureBuffer::GpuResources::~GpuResources() {}

bool D3D11PictureBuffer::GpuResources::Init(
    base::Callback<gpu::GpuCommandBufferStub*()> get_stub_cb,
    int level,
    const std::vector<gpu::Mailbox> mailboxes,
    GLenum target,
    gfx::Size size,
    base::win::ScopedComPtr<ID3D11Texture2D> angle_texture,
    int textures_per_picture) {
  gpu::GpuCommandBufferStub* stub = get_stub_cb.Run();

  if (!MakeContextCurrent(stub))
    return false;

  // TODO(liberato): see GpuVideoFrameFactory.
  // stub_->AddDestructionObserver(this);

  auto decoder_helper = GLES2DecoderHelper::Create(stub->decoder());
  gpu::gles2::ContextGroup* group = stub->decoder()->GetContextGroup();
  gpu::gles2::MailboxManager* mailbox_manager = group->mailbox_manager();
  gpu::gles2::TextureManager* texture_manager = group->texture_manager();
  if (!texture_manager)
    return false;

  // Create the textures and attach them to the mailboxes.
  std::vector<uint32_t> service_ids;
  for (int texture_idx = 0; texture_idx < textures_per_picture; texture_idx++) {
    texture_refs_.push_back(decoder_helper->CreateTexture(
        target, GL_RGBA, size.width(), size.height(), GL_RGBA,
        GL_UNSIGNED_BYTE));
    service_ids.push_back(texture_refs_[texture_idx]->service_id());

    mailbox_manager->ProduceTexture(mailboxes[texture_idx],
                                    texture_refs_[texture_idx]->texture());
  }

  // Create the stream for zero-copy use by gl.
  EGLDisplay egl_display = gl::GLSurfaceEGL::GetHardwareDisplay();
  const EGLint stream_attributes[] = {
      EGL_CONSUMER_LATENCY_USEC_KHR,
      0,
      EGL_CONSUMER_ACQUIRE_TIMEOUT_USEC_KHR,
      0,
      EGL_NONE,
  };
  EGLStreamKHR stream = eglCreateStreamKHR(egl_display, stream_attributes);
  if (!stream)
    return false;

  // |stream| will be destroyed when the GLImage is.
  scoped_refptr<gl::GLImage> gl_image =
      base::MakeRefCounted<gl::GLImageDXGI>(size, stream);
  gl::ScopedActiveTexture texture0(GL_TEXTURE0);
  gl::ScopedTextureBinder texture0_binder(GL_TEXTURE_EXTERNAL_OES,
                                          service_ids[0]);
  gl::ScopedActiveTexture texture1(GL_TEXTURE1);
  gl::ScopedTextureBinder texture1_binder(GL_TEXTURE_EXTERNAL_OES,
                                          service_ids[1]);

  EGLAttrib consumer_attributes[] = {
      EGL_COLOR_BUFFER_TYPE,
      EGL_YUV_BUFFER_EXT,
      EGL_YUV_NUMBER_OF_PLANES_EXT,
      2,
      EGL_YUV_PLANE0_TEXTURE_UNIT_NV,
      0,
      EGL_YUV_PLANE1_TEXTURE_UNIT_NV,
      1,
      EGL_NONE,
  };
  EGLBoolean result = eglStreamConsumerGLTextureExternalAttribsNV(
      egl_display, stream, consumer_attributes);
  if (!result)
    return false;

  EGLAttrib producer_attributes[] = {
      EGL_NONE,
  };

  result = eglCreateStreamProducerD3DTextureNV12ANGLE(egl_display, stream,
                                                      producer_attributes);

  EGLAttrib frame_attributes[] = {
      EGL_D3D_TEXTURE_SUBRESOURCE_ID_ANGLE, level, EGL_NONE,
  };

  result = eglStreamPostD3DTextureNV12ANGLE(
      egl_display, stream, static_cast<void*>(angle_texture.Get()),
      frame_attributes);
  if (!result)
    return false;

  result = eglStreamConsumerAcquireKHR(egl_display, stream);

  if (!result)
    return false;
  gl::GLImageDXGI* gl_image_dxgi =
      static_cast<gl::GLImageDXGI*>(gl_image.get());

  gl_image_dxgi->SetTexture(angle_texture, level);

  // Bind the image to each texture.
  for (int texture_idx = 0; texture_idx < texture_refs_.size(); texture_idx++) {
    texture_manager->SetLevelImage(texture_refs_[texture_idx].get(),
                                   GL_TEXTURE_EXTERNAL_OES, 0, gl_image.get(),
                                   gpu::gles2::Texture::ImageState::BOUND);
  }

  return true;
}

}  // namespace media
