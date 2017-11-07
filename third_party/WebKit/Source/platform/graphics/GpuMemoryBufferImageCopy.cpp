// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/graphics/GpuMemoryBufferImageCopy.h"
#include "gpu/command_buffer/client/gles2_interface.h"
#include "gpu/command_buffer/client/gpu_memory_buffer_manager.h"
#include "platform/graphics/GraphicsTypes3D.h"
#include "platform/graphics/Image.h"
#include "platform/graphics/StaticBitmapImage.h"
#include "public/platform/Platform.h"
#include "public/platform/WebGraphicsContext3DProvider.h"

#include "platform/instrumentation/tracing/TraceEvent.h"

namespace blink {

GpuMemoryBufferImageCopy::GpuMemoryBufferImageCopy()
    : contextProvider_(Platform::Current()
                           ->CreateSharedOffscreenGraphicsContext3DProvider()) {
}

bool GpuMemoryBufferImageCopy::EnsureMemoryBuffer(int width, int height) {
  // Create a new memorybuffer if the size has changed, or we don't have one.
  if (lastWidth_ != width || lastHeight_ != height || !gpuMemoryBuffer_) {
    gpu::GpuMemoryBufferManager* gpuMemoryBufferManager =
        Platform::Current()->GetGpuMemoryBufferManager();
    if (!gpuMemoryBufferManager)
      return false;

    gpuMemoryBuffer_ = gpuMemoryBufferManager->CreateGpuMemoryBuffer(
        gfx::Size(width, height), gfx::BufferFormat::RGBA_8888,
        gfx::BufferUsage::SCANOUT, gpu::kNullSurfaceHandle);
    if (!gpuMemoryBuffer_)
      return false;

    lastWidth_ = width;
    lastHeight_ = height;
  }
  return true;
}

gfx::GpuMemoryBuffer* GpuMemoryBufferImageCopy::CopyImage(RefPtr<Image> image) {
  if (!image)
    return nullptr;

  TRACE_EVENT0("gpu", "GpuMemoryBufferImageCopy::CopyImage");

  gpu::gles2::GLES2Interface* gl;
  {
    TRACE_EVENT0("gpu", "GpuMemoryBufferImageCopy::CopyImage-getGL");
    gl = contextProvider_->ContextGL();
  }
  if (!gl)
    return nullptr;

  int width = image->width();
  int height = image->height();
  if (!EnsureMemoryBuffer(width, height))
    return nullptr;

  // Bind the write framebuffer to our memory buffer.
  GLuint imageId = gl->CreateImageCHROMIUM(gpuMemoryBuffer_->AsClientBuffer(),
                                           width, height, GL_RGBA);
  if (!imageId)
    return nullptr;
  GLuint textureId;
  gl->GenTextures(1, &textureId);
  GLenum target = GL_TEXTURE_2D;
  {
    gl->BindTexture(target, textureId);
    gl->TexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    gl->TexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    gl->TexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gl->TexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    gl->BindTexImage2DCHROMIUM(target, imageId);
  }
  gl->BindTexture(GL_TEXTURE_2D, 0);
  GLuint frameBuffer;
  gl->GenFramebuffers(1, &frameBuffer);
  gl->BindFramebuffer(GL_DRAW_FRAMEBUFFER, frameBuffer);
  gl->FramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target,
                           textureId, 0);

  // Bind the read framebuffer to our image.
  StaticBitmapImage* static_image =
      static_cast<StaticBitmapImage*>(image.get());
  static_image->EnsureMailbox(kVerifiedSyncToken);
  auto mailbox = static_image->GetMailbox();
  auto sync_token = static_image->GetSyncToken();
  gl->WaitSyncTokenCHROMIUM(sync_token.GetData());
  GLuint source_texture_id =
      gl->CreateAndConsumeTextureCHROMIUM(GL_TEXTURE_2D, mailbox.name);
  gl->BindTexture(GL_TEXTURE_2D, 0);
  GLuint readFrameBuffer;
  gl->GenFramebuffers(1, &readFrameBuffer);
  gl->BindFramebuffer(GL_READ_FRAMEBUFFER, readFrameBuffer);
  gl->FramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target,
                           source_texture_id, 0);

  // Copy the read framebuffer to the draw framebuffer.
  gl->BlitFramebufferCHROMIUM(0, 0, width, height, 0, 0, width, height,
                              GL_COLOR_BUFFER_BIT, GL_LINEAR);

  // Cleanup the read framebuffer, associated image and texture.
  gl->BindFramebuffer(GL_FRAMEBUFFER, 0);
  gl->DeleteFramebuffers(1, &readFrameBuffer);
  gl->BindTexture(GL_TEXTURE_2D, 0);
  gl->DeleteTextures(1, &source_texture_id);

  // Cleanup the draw framebuffer, associated image and texture.
  gl->DeleteFramebuffers(1, &frameBuffer);
  gl->BindTexture(target, textureId);
  gl->ReleaseTexImage2DCHROMIUM(target, imageId);
  gl->DestroyImageCHROMIUM(imageId);
  gl->DeleteTextures(1, &textureId);
  gl->BindTexture(target, 0);

  gl->Flush();
  return gpuMemoryBuffer_.get();
}

}  // namespace blink
