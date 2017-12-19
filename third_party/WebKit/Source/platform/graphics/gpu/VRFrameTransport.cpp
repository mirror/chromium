// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/graphics/gpu/VRFrameTransport.h"

#include "build/build_config.h"
#include "device/vr/vr_service.mojom-blink.h"
#include "gpu/command_buffer/client/gles2_interface.h"
#include "gpu/command_buffer/common/mailbox_holder.h"
#include "mojo/public/cpp/system/platform_handle.h"
#include "platform/graphics/GpuMemoryBufferImageCopy.h"
#include "platform/graphics/StaticBitmapImage.h"
#include "ui/gfx/gpu_fence.h"

namespace blink {

VRFrameTransport::VRFrameTransport(gpu::gles2::GLES2Interface* gl)
    : submit_frame_client_binding_(this), context_gl_(gl) {}

VRFrameTransport::~VRFrameTransport() {}

void VRFrameTransport::PresentChange() {
  frame_copier_ = nullptr;
}

void VRFrameTransport::SetTransportOptions(
    device::mojom::blink::VRDisplayFrameTransportOptionsPtr transport_options) {
  transport_options_ = std::move(transport_options);
}

device::mojom::blink::VRSubmitFrameClientPtr
VRFrameTransport::GetSubmitFrameClient() {
  device::mojom::blink::VRSubmitFrameClientPtr submit_frame_client;
  submit_frame_client_binding_.Close();
  submit_frame_client_binding_.Bind(mojo::MakeRequest(&submit_frame_client));
  return submit_frame_client;
}

void VRFrameTransport::FramePreImage() {
  frame_wait_time_ = WTF::TimeDelta();

  if (transport_options_->waitForGpuFence) {
    frame_wait_time_ += WaitForGpuFence();
    DVLOG(3) << "CreateClientGpuFenceCHROMIUM(prev_frame_fence_)";
    GLuint id = context_gl_->CreateClientGpuFenceCHROMIUM(
        prev_frame_fence_->AsClientGpuFence());
    context_gl_->WaitGpuFenceCHROMIUM(id);
    context_gl_->DestroyGpuFenceCHROMIUM(id);
    prev_frame_fence_.reset();
  }
}

void VRFrameTransport::FrameSubmit(
    device::mojom::blink::VRPresentationProvider* vr_presentation_provider,
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
    TRACE_EVENT0("gpu", "VRDisplay::CopyImage");
    // Update last_transfer_succeeded_ value. This should usually complete
    // without waiting.
    if (transport_options_->waitForTransferNotification)
      WaitForPreviousTransfer();
    if (!frame_copier_ || !last_transfer_succeeded_) {
      frame_copier_ = std::make_unique<GpuMemoryBufferImageCopy>(context_gl_);
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
    TRACE_EVENT_BEGIN0("gpu", "VRDisplay::EnsureMailbox");
    static_image->EnsureMailbox(kVerifiedSyncToken, GL_NEAREST);
    TRACE_EVENT_END0("gpu", "VRDisplay::EnsureMailbox");

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
    TRACE_EVENT_BEGIN0("gpu", "VRDisplay::GetMailbox");
    auto mailbox = static_image->GetMailbox();
    TRACE_EVENT_END0("gpu", "VRDisplay::GetMailbox");
    auto sync_token = static_image->GetSyncToken();

    TRACE_EVENT_BEGIN0("gpu", "VRDisplay::SubmitFrame");
    vr_presentation_provider->SubmitFrame(
        vr_frame_id, gpu::MailboxHolder(mailbox, sync_token, GL_TEXTURE_2D),
        frame_wait_time_);
    TRACE_EVENT_END0("gpu", "VRDisplay::SubmitFrame");
  } else {
    NOTREACHED() << "Unimplemented frame transport method";
  }

  pending_previous_frame_render_ = true;
}

void VRFrameTransport::OnSubmitFrameTransferred(bool success) {
  DVLOG(3) << __FUNCTION__;
  pending_submit_frame_ = false;
  last_transfer_succeeded_ = success;
}

void VRFrameTransport::WaitForPreviousTransfer() {
  TRACE_EVENT0("gpu", "waitForPreviousTransferToFinish");
  while (pending_submit_frame_) {
    if (!submit_frame_client_binding_.WaitForIncomingMethodCall()) {
      DLOG(ERROR) << "Failed to receive SubmitFrame response";
      break;
    }
  }
}

void VRFrameTransport::OnSubmitFrameRendered() {
  DVLOG(3) << __FUNCTION__;
  pending_previous_frame_render_ = false;
}

WTF::TimeDelta VRFrameTransport::WaitForPreviousRenderToFinish() {
  TRACE_EVENT0("gpu", "waitForPreviousRenderToFinish");
  WTF::TimeTicks start = WTF::CurrentTimeTicks();
  while (pending_previous_frame_render_) {
    if (!submit_frame_client_binding_.WaitForIncomingMethodCall()) {
      DLOG(ERROR) << "Failed to receive SubmitFrame response";
      break;
    }
  }
  return WTF::CurrentTimeTicks() - start;
}

void VRFrameTransport::OnSubmitFrameGpuFence(
    const gfx::GpuFenceHandle& handle) {
  prev_frame_fence_ = std::make_unique<gfx::GpuFence>(handle);
}

WTF::TimeDelta VRFrameTransport::WaitForGpuFence() {
  TRACE_EVENT0("gpu", "waitForGpuFence");
  WTF::TimeTicks start = WTF::CurrentTimeTicks();
  while (!prev_frame_fence_) {
    if (!submit_frame_client_binding_.WaitForIncomingMethodCall()) {
      DLOG(ERROR) << "Failed to receive SubmitFrame response";
      break;
    }
  }
  return WTF::CurrentTimeTicks() - start;
}

void VRFrameTransport::Trace(blink::Visitor* visitor) {}

}  // namespace blink
