// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef VRFrameTransport_h
#define VRFrameTransport_h

#include "device/vr/vr_service.mojom-blink.h"
#include "mojo/public/cpp/bindings/binding.h"
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

class PLATFORM_EXPORT GpuMemoryBufferImageCopy;
class Image;

class VRFrameTransport final
    : public GarbageCollectedFinalized<VRFrameTransport>,
      public device::mojom::blink::VRSubmitFrameClient {
 public:
  VRFrameTransport(gpu::gles2::GLES2Interface*);
  ~VRFrameTransport();

  device::mojom::blink::VRSubmitFrameClientPtr GetSubmitFrameClient();

  void PresentChange();

  void SetTransportOptions(
      device::mojom::blink::VRDisplayFrameTransportOptionsPtr);

  // Call before finalizing the frame's image snapshot.
  void FramePreImage();

  void FrameSubmit(device::mojom::blink::VRPresentationProvider*,
                   DrawingBuffer::Client*,
                   scoped_refptr<Image> image_ref,
                   int16_t vr_frame_id,
                   bool needs_copy);

  virtual void Trace(blink::Visitor*);

 private:
  void WaitForPreviousTransfer();
  WTF::TimeDelta WaitForPreviousRenderToFinish();
  WTF::TimeDelta WaitForGpuFence();

  // VRSubmitFrameClient
  void OnSubmitFrameTransferred(bool success) override;
  void OnSubmitFrameRendered() override;
  void OnSubmitFrameGpuFence(const gfx::GpuFenceHandle&) override;

  mojo::Binding<device::mojom::blink::VRSubmitFrameClient>
      submit_frame_client_binding_;

  gpu::gles2::GLES2Interface* context_gl_ = nullptr;

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
};

}  // namespace blink

#endif  // VRFrameTransport_h
