// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef XRFrameTransport_h
#define XRFrameTransport_h

#include "device/vr/vr_service.mojom-blink.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "platform/PlatformExport.h"
#include "platform/graphics/gpu/DrawingBuffer.h"
#include "platform/heap/Handle.h"
#include "platform/wtf/Time.h"
#include "public/platform/WebGraphicsContext3DProvider.h"

namespace gfx {
class GpuFence;
}

namespace gpu {
namespace gles2 {
class GLES2Interface;
}
}  // namespace gpu

namespace blink {

class GpuMemoryBufferImageCopy;
class Image;

class PLATFORM_EXPORT XRFrameTransport final
    : public GarbageCollectedFinalized<XRFrameTransport>,
      public device::mojom::blink::VRSubmitFrameClient {
 public:
  explicit XRFrameTransport(int sample_count);
  ~XRFrameTransport();

  device::mojom::blink::VRSubmitFrameClientPtr GetSubmitFrameClient();

  void PresentChange(gpu::gles2::GLES2Interface*, bool is_presenting);

  void SetTransportOptions(
      device::mojom::blink::VRDisplayFrameTransportOptionsPtr);

  bool DrawingIntoFBO();

  GLuint BindAHBToBufferHolder(
      gpu::gles2::GLES2Interface*,
      DrawingBuffer::Client*,
      const base::Optional<gpu::MailboxHolder>& buffer_holder,
      const blink::WebSize& buffer_size);

  // Call before finalizing the frame's image snapshot.
  void FramePreImage(gpu::gles2::GLES2Interface*);

  void FrameSubmit(device::mojom::blink::VRPresentationProvider*,
                   gpu::gles2::GLES2Interface*,
                   DrawingBuffer::Client*,
                   scoped_refptr<Image> image_ref,
                   int16_t vr_frame_id,
                   bool needs_copy);

  virtual void Trace(blink::Visitor*);

 private:
  void WaitForPreviousTransfer();
  WTF::TimeDelta WaitForPreviousRenderToFinish();
  WTF::TimeDelta WaitForGpuFenceReceived();

  // VRSubmitFrameClient
  void OnSubmitFrameTransferred(bool success) override;
  void OnSubmitFrameRendered() override;
  void OnSubmitFrameGpuFence(const gfx::GpuFenceHandle&) override;

  mojo::Binding<device::mojom::blink::VRSubmitFrameClient>
      submit_frame_client_binding_;

  // Used to keep the image alive until the next frame if using
  // waitForPreviousTransferToFinish.
  scoped_refptr<Image> previous_image_;

  bool pending_submit_frame_ = false;
  bool last_transfer_succeeded_ = false;
  bool pending_previous_frame_render_ = false;
  std::unique_ptr<gfx::GpuFence> prev_frame_fence_;
  WTF::TimeDelta frame_wait_time_;

  device::mojom::blink::VRDisplayFrameTransportOptionsPtr transport_options_;

  std::unique_ptr<GpuMemoryBufferImageCopy> frame_copier_;

  void AllocateAHBResources(gpu::gles2::GLES2Interface*);
  void ReleaseAHBResources(gpu::gles2::GLES2Interface*);
  void CreateAndBindAHBImage(
      gpu::gles2::GLES2Interface*, DrawingBuffer::Client*,
      const base::Optional<gpu::MailboxHolder>& buffer_holder);

  int ahb_sample_count_;
  int ahb_width_;
  int ahb_height_;
  GLuint ahb_fbo_ = 0;
  GLuint ahb_depthbuffer_ = 0;
  GLuint ahb_texture_ = 0;
};

}  // namespace blink

#endif  // XRFrameTransport_h
