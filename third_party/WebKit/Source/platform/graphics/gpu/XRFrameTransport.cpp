// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/graphics/gpu/XRFrameTransport.h"

#include "build/build_config.h"
#include "device/vr/vr_service.mojom-blink.h"
#include "gpu/command_buffer/client/gles2_interface.h"
#include "gpu/command_buffer/common/mailbox_holder.h"
#include "mojo/public/cpp/system/platform_handle.h"
#include "platform/graphics/GpuMemoryBufferImageCopy.h"
#include "platform/graphics/StaticBitmapImage.h"
#include "ui/gfx/gpu_fence.h"

#if 0
#undef DVLOG
#define DVLOG(x) LOG(INFO)
#endif

// WARNING: error checking has a huge performance cost. Only
// enable this when actively debugging problems.
#define EXPENSIVE_GL_ERROR_CHECKING 0

#if EXPENSIVE_GL_ERROR_CHECKING
#define CHECK_ERR                                            \
  do {                                                       \
    DVLOG(2) << __FUNCTION__ << ";;; error check START";     \
    GLint err;                                               \
    while ((err = gl->GetError()) != GL_NO_ERROR) { \
      LOG(INFO) << __FUNCTION__ << ";;; GL ERROR " << err;   \
    }                                                        \
    DVLOG(2) << __FUNCTION__ << ";;; error check DONE";      \
  } while (0)
#else
#define CHECK_ERR \
  do {            \
  } while (0)
#endif

namespace blink {

XRFrameTransport::XRFrameTransport(int sample_count) :
    submit_frame_client_binding_(this),
    ahb_sample_count_(sample_count) {}

XRFrameTransport::~XRFrameTransport() {}

void XRFrameTransport::PresentChange(gpu::gles2::GLES2Interface* gl,
                                     bool is_presenting) {
  DVLOG(1) << __FUNCTION__ << ": is_presenting=" << is_presenting;
  frame_copier_ = nullptr;
  if (!is_presenting) {
    ReleaseAHBResources(gl);
  }
}

void XRFrameTransport::SetTransportOptions(
    device::mojom::blink::VRDisplayFrameTransportOptionsPtr transport_options) {
  transport_options_ = std::move(transport_options);
}

device::mojom::blink::VRSubmitFrameClientPtr
XRFrameTransport::GetSubmitFrameClient() {
  device::mojom::blink::VRSubmitFrameClientPtr submit_frame_client;
  submit_frame_client_binding_.Close();
  submit_frame_client_binding_.Bind(mojo::MakeRequest(&submit_frame_client));
  return submit_frame_client;
}

bool XRFrameTransport::DrawingIntoFBO() {
  switch (transport_options_->transportMethod) {
    case device::mojom::blink::VRDisplayFrameTransportMethod::
        SUBMIT_AS_TEXTURE_HANDLE:
    case device::mojom::blink::VRDisplayFrameTransportMethod::
        SUBMIT_AS_MAILBOX_HOLDER:
      return false;
    case device::mojom::blink::VRDisplayFrameTransportMethod::
        DRAW_INTO_TEXTURE_MAILBOX:
      return true;
    default:
      NOTREACHED();
      return false;
  }
}

void XRFrameTransport::FramePreImage(gpu::gles2::GLES2Interface* gl) {
  frame_wait_time_ = WTF::TimeDelta();

  if (transport_options_->waitForGpuFence) {
    frame_wait_time_ += WaitForGpuFenceReceived();
    // Receiving the fence can fail if there are Mojo communication issues,
    // for example when shutting down. This generally means that submitting
    // the frame would also fail.
    if (!prev_frame_fence_)
      return;
    // Now that we have received the GpuFence, send it to the GPU service
    // process and ask it to do an asynchronous server wait.
    DVLOG(3) << "CreateClientGpuFenceCHROMIUM(prev_frame_fence_)";
    GLuint id =
        gl->CreateClientGpuFenceCHROMIUM(prev_frame_fence_->AsClientGpuFence());
    gl->WaitGpuFenceCHROMIUM(id);
    gl->DestroyGpuFenceCHROMIUM(id);
    prev_frame_fence_.reset();
  }

  if (transport_options_->transportMethod ==
             device::mojom::blink::VRDisplayFrameTransportMethod::
                 DRAW_INTO_TEXTURE_MAILBOX) {
#if 0
    static int fnum = 0;
    ++fnum;
    gl->ClearColor(0.25 + 0.75 * (fnum % 64) / 64.0, 0.0, 0.1, 1.0);
    gl->Clear(GL_COLOR_BUFFER_BIT);
#endif

    TRACE_EVENT0("gpu", "XRFrameTransport::FinishDraw");
    CHECK_ERR;
    DVLOG(2) << __FUNCTION__;
    // We're done with the framebuffer, the caller will unbind it after this
    // call. Give a hint to the driver that we're done with the attachments.
    gl->BindFramebuffer(GL_FRAMEBUFFER, ahb_fbo_);
    CHECK_ERR;
    // Discard attachments to tell GL we're done with them.
    // Not sure if we can discard COLOR_ATTACHMENT0, the content is still
    // needed.
    const GLenum kAttachments[] = {// GL_COLOR_ATTACHMENT0,
                                   GL_DEPTH_ATTACHMENT, GL_STENCIL_ATTACHMENT};
    gl->DiscardFramebufferEXT(
        GL_FRAMEBUFFER, sizeof(kAttachments) / sizeof(kAttachments[0]),
        kAttachments);
    CHECK_ERR;
  }
}

void XRFrameTransport::FrameSubmit(
    device::mojom::blink::VRPresentationProvider* vr_presentation_provider,
    gpu::gles2::GLES2Interface* gl,
    DrawingBuffer::Client* drawing_buffer_client,
    scoped_refptr<Image> image_ref,
    int16_t vr_frame_id,
    bool needs_copy) {
  DCHECK(transport_options_);

  if (transport_options_->transportMethod ==
      device::mojom::blink::VRDisplayFrameTransportMethod::
          SUBMIT_AS_TEXTURE_HANDLE) {
#if defined(OS_WIN)
    // Currently, we assume that this transport needs a copy.
    DCHECK(needs_copy);
    TRACE_EVENT0("gpu", "XRFrameTransport::CopyImage");
    // Update last_transfer_succeeded_ value. This should usually complete
    // without waiting.
    if (transport_options_->waitForTransferNotification)
      WaitForPreviousTransfer();
    if (!frame_copier_ || !last_transfer_succeeded_) {
      frame_copier_ = std::make_unique<GpuMemoryBufferImageCopy>(gl);
    }
    auto gpu_memory_buffer = frame_copier_->CopyImage(image_ref.get());
    drawing_buffer_client->DrawingBufferClientRestoreTexture2DBinding();
    drawing_buffer_client->DrawingBufferClientRestoreFramebufferBinding();
    drawing_buffer_client->DrawingBufferClientRestoreRenderbufferBinding();

    // We can fail to obtain a GpuMemoryBuffer if we don't have GPU support, or
    // for some out-of-memory situations.
    // TODO(billorr): Consider whether we should just drop the frame or exit
    // presentation.
    if (gpu_memory_buffer) {
      // We decompose the cloned handle, and use it to create a
      // mojo::ScopedHandle which will own cleanup of the handle, and will be
      // passed over IPC.
      gfx::GpuMemoryBufferHandle gpu_handle =
          CloneHandleForIPC(gpu_memory_buffer->GetHandle());
      vr_presentation_provider->SubmitFrameWithTextureHandle(
          vr_frame_id, mojo::WrapPlatformFile(gpu_handle.handle.GetHandle()));
    }
#else
    NOTIMPLEMENTED();
#endif
  } else if (transport_options_->transportMethod ==
             device::mojom::blink::VRDisplayFrameTransportMethod::
                 SUBMIT_AS_MAILBOX_HOLDER) {
    // Currently, this transport assumes we don't need to make a separate copy
    // of the canvas content.
    DCHECK(!needs_copy);

    // The AcceleratedStaticBitmapImage must be kept alive until the
    // mailbox is used via createAndConsumeTextureCHROMIUM, the mailbox
    // itself does not keep it alive. We must keep a reference to the
    // image until the mailbox was consumed.
    StaticBitmapImage* static_image =
        static_cast<StaticBitmapImage*>(image_ref.get());
    TRACE_EVENT_BEGIN0("gpu", "XRFrameTransport::EnsureMailbox");
    static_image->EnsureMailbox(kVerifiedSyncToken, GL_NEAREST);
    TRACE_EVENT_END0("gpu", "XRFrameTransport::EnsureMailbox");

    // Conditionally wait for the previous render to finish. A late wait here
    // attempts to overlap work in parallel with the previous frame's
    // rendering. This is used if submitting fully rendered frames to GVR, but
    // is susceptible to bad GPU scheduling if the new frame competes with the
    // previous frame's incomplete rendering.
    if (transport_options_->waitForRenderNotification)
      frame_wait_time_ += WaitForPreviousRenderToFinish();

    // Save a reference to the image to keep it alive until next frame,
    // but first wait for the transfer to finish before overwriting it.
    // Usually this check is satisfied without waiting.
    if (transport_options_->waitForTransferNotification)
      WaitForPreviousTransfer();
    previous_image_ = std::move(image_ref);

    // Create mailbox and sync token for transfer.
    TRACE_EVENT_BEGIN0("gpu", "XRFrameTransport::GetMailbox");
    auto mailbox = static_image->GetMailbox();
    TRACE_EVENT_END0("gpu", "XRFrameTransport::GetMailbox");
    auto sync_token = static_image->GetSyncToken();

    TRACE_EVENT_BEGIN0("gpu", "XRFrameTransport::SubmitFrame");
    vr_presentation_provider->SubmitFrame(
        vr_frame_id, gpu::MailboxHolder(mailbox, sync_token, GL_TEXTURE_2D),
        frame_wait_time_);
    TRACE_EVENT_END0("gpu", "XRFrameTransport::SubmitFrame");
  } else if (transport_options_->transportMethod ==
             device::mojom::blink::VRDisplayFrameTransportMethod::
                 DRAW_INTO_TEXTURE_MAILBOX) {
    gpu::SyncToken sync_token;

    {
      TRACE_EVENT0("gpu", "GenSyncTokenCHROMIUM");
      gl->GenSyncTokenCHROMIUM(sync_token.GetData());
      CHECK_ERR;
    }
    // LOG(INFO) << __FUNCTION__ << ";;; frame=" << vr_frame_id_;
    LOG(INFO) << __FUNCTION__ << ";;; frame_wait_time_=" << frame_wait_time_;
    vr_presentation_provider->SubmitFrameZeroCopy3(vr_frame_id, sync_token,
                                                    frame_wait_time_);
    CHECK_ERR;
  } else {
    NOTREACHED() << "Unimplemented frame transport method";
  }

  pending_previous_frame_render_ = true;
}

void XRFrameTransport::OnSubmitFrameTransferred(bool success) {
  DVLOG(3) << __FUNCTION__;
  pending_submit_frame_ = false;
  last_transfer_succeeded_ = success;
}

void XRFrameTransport::WaitForPreviousTransfer() {
  TRACE_EVENT0("gpu", "waitForPreviousTransferToFinish");
  while (pending_submit_frame_) {
    if (!submit_frame_client_binding_.WaitForIncomingMethodCall()) {
      DLOG(ERROR) << __FUNCTION__ << ": Failed to receive response";
      break;
    }
  }
}

void XRFrameTransport::OnSubmitFrameRendered() {
  DVLOG(3) << __FUNCTION__;
  pending_previous_frame_render_ = false;
}

WTF::TimeDelta XRFrameTransport::WaitForPreviousRenderToFinish() {
  TRACE_EVENT0("gpu", "waitForPreviousRenderToFinish");
  WTF::TimeTicks start = WTF::CurrentTimeTicks();
  while (pending_previous_frame_render_) {
    if (!submit_frame_client_binding_.WaitForIncomingMethodCall()) {
      DLOG(ERROR) << __FUNCTION__ << ": Failed to receive response";
      break;
    }
  }
  return WTF::CurrentTimeTicks() - start;
}

void XRFrameTransport::OnSubmitFrameGpuFence(
    const gfx::GpuFenceHandle& handle) {
  prev_frame_fence_ = std::make_unique<gfx::GpuFence>(handle);
}

WTF::TimeDelta XRFrameTransport::WaitForGpuFenceReceived() {
  TRACE_EVENT0("gpu", "WaitForGpuFenceReceived");
  WTF::TimeTicks start = WTF::CurrentTimeTicks();
  while (!prev_frame_fence_) {
    if (!submit_frame_client_binding_.WaitForIncomingMethodCall()) {
      DLOG(ERROR) << __FUNCTION__ << ": Failed to receive response";
      break;
    }
  }
  return WTF::CurrentTimeTicks() - start;
}

void XRFrameTransport::AllocateAHBResources(gpu::gles2::GLES2Interface* gl) {
  ahb_fbo_ = 0;
  ahb_depthbuffer_ = 0;
  ahb_texture_ = 0;
  gl->GenFramebuffers(1, &ahb_fbo_);
  CHECK_ERR;
  gl->GenRenderbuffers(1, &ahb_depthbuffer_);
  CHECK_ERR;
  // ahb_texture_ is created per frame and lazily deleted.
}

void XRFrameTransport::ReleaseAHBResources(gpu::gles2::GLES2Interface* gl) {
  if (ahb_texture_ > 0) {
    gl->DeleteTextures(1, &ahb_texture_);
    CHECK_ERR;
    ahb_texture_ = 0;
  }
  if (ahb_depthbuffer_ > 0) {
    gl->DeleteRenderbuffers(1, &ahb_depthbuffer_);
    CHECK_ERR;
    ahb_depthbuffer_ = 0;
  }
  if (ahb_fbo_ > 0) {
    gl->DeleteFramebuffers(1, &ahb_fbo_);
    CHECK_ERR;
    ahb_fbo_ = 0;
  }
}

void XRFrameTransport::CreateAndBindAHBImage(
    gpu::gles2::GLES2Interface* gl,
    DrawingBuffer::Client* drawing_buffer_client,
    const base::Optional<gpu::MailboxHolder>& buffer_holder) {
  TRACE_EVENT_BEGIN0("gpu", "XRFrameTransport::BindTexImage");

  // Delete the previously-used texture as late as possible. Rebinding an
  // existing texture blocks for rendering to complete on
  // Texture::SetLevelInfo's "info.image = 0" in texture_manager.cc?
  if (ahb_texture_ > 0) {
    gl->DeleteTextures(1, &ahb_texture_);
  }
  ahb_texture_ = gl->CreateAndConsumeTextureCHROMIUM(buffer_holder->mailbox.name);
  LOG(INFO) << __FUNCTION__ << ";;; texture=" << ahb_texture_;
  CHECK_ERR;
  gl->BindTexture(GL_TEXTURE_2D, ahb_texture_);
  gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                             GL_NEAREST);
  gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                             GL_NEAREST);
  gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                             GL_CLAMP_TO_EDGE);
  gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                             GL_CLAMP_TO_EDGE);
  CHECK_ERR;
  TRACE_EVENT_END0("gpu", "XRFrameTransport::BindTexImage");

  gl->FramebufferTexture2DMultisampleEXT(
      GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ahb_texture_, 0,
      ahb_sample_count_);
  CHECK_ERR;

#if EXPENSIVE_GL_ERROR_CHECKING
  auto fbstatus = gl->CheckFramebufferStatus(GL_FRAMEBUFFER);
  if (fbstatus != GL_FRAMEBUFFER_COMPLETE) {
    LOG(ERROR) << __FUNCTION__
               << ";;; FRAMEBUFFER NOT COMPLETE, fbstatus=0x" << std::hex << fbstatus;
    gl->BindFramebuffer(GL_FRAMEBUFFER, 0);
  }
  CHECK_ERR;
#endif

  drawing_buffer_client->DrawingBufferClientRestoreTexture2DBinding();
}

GLuint XRFrameTransport::BindAHBToBufferHolder(
    gpu::gles2::GLES2Interface* gl,
    DrawingBuffer::Client* drawing_buffer_client,
    const base::Optional<gpu::MailboxHolder>& buffer_holder,
    const blink::WebSize& buffer_size) {
  CHECK_ERR;

  TRACE_EVENT_BEGIN0("gpu", "XRFrameTransport::WaitSyncToken");
  gl->WaitSyncTokenCHROMIUM(buffer_holder->sync_token.GetConstData());
  TRACE_EVENT_END0("gpu", "XRFrameTransport::WaitSyncToken");

  if (ahb_fbo_ == 0 || ahb_depthbuffer_ == 0) {
    AllocateAHBResources(gl);
    // Set invalid size to force rebinding.
    ahb_width_ = -1;
    ahb_height_ = -1;
  }

  gl->BindFramebuffer(GL_FRAMEBUFFER, ahb_fbo_);
  CHECK_ERR;

  if (buffer_size.width != ahb_width_ || buffer_size.height != ahb_height_) {
    LOG(INFO) << __FUNCTION__ << ";;; width=" << buffer_size.width << " height=" << buffer_size.height;
    gl->BindFramebuffer(GL_FRAMEBUFFER, ahb_fbo_);
    CHECK_ERR;
    gl->BindRenderbuffer(GL_RENDERBUFFER, ahb_depthbuffer_);
    CHECK_ERR;

    gl->RenderbufferStorageMultisampleEXT(
        GL_RENDERBUFFER, ahb_sample_count_, GL_DEPTH24_STENCIL8_OES,
        buffer_size.width, buffer_size.height);
    CHECK_ERR;

    gl->FramebufferRenderbuffer(GL_FRAMEBUFFER,
                                         GL_DEPTH_STENCIL_ATTACHMENT,
                                         GL_RENDERBUFFER, ahb_depthbuffer_);

    CHECK_ERR;
    ahb_width_ = buffer_size.width;
    ahb_height_ = buffer_size.height;

    drawing_buffer_client->DrawingBufferClientRestoreRenderbufferBinding();
  }

#if 0
  // Invalidate all attachments, we're drawing fresh content. Redundant?
  // TODO(klausw): research and/or benchmark if this is helpful.
  const GLenum kAttachments[] = {GL_COLOR_ATTACHMENT0, GL_DEPTH_ATTACHMENT,
                                 GL_STENCIL_ATTACHMENT};
  gl->DiscardFramebufferEXT(
      GL_FRAMEBUFFER, sizeof(kAttachments) / sizeof(kAttachments[0]),
      kAttachments);
  CHECK_ERR;
#endif

  CreateAndBindAHBImage(gl, drawing_buffer_client, buffer_holder);
  CHECK_ERR;

  // TODO(klausw): handle preserveBuffer=true

  return ahb_fbo_;
}


void XRFrameTransport::Trace(blink::Visitor* visitor) {}

}  // namespace blink
