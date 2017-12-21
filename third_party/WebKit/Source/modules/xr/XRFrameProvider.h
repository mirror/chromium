// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef XRFrameProvider_h
#define XRFrameProvider_h

#include "core/imagebitmap/ImageBitmap.h"
#include "device/vr/vr_service.mojom-blink.h"
#include "gpu/command_buffer/common/mailbox_holder.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "platform/bindings/ScriptWrappable.h"
#include "platform/heap/Handle.h"
#include "platform/wtf/Forward.h"

namespace blink {

class ScriptPromiseResolver;
class XRDevice;
class XRSession;
class XRWebGLLayer;

// This class manages requesting and dispatching frame updates, which includes
// pose information for a given XRDevice.
class XRFrameProvider final : public GarbageCollectedFinalized<XRFrameProvider>,
                              public device::mojom::blink::VRSubmitFrameClient {
 public:
  class ImageSource {
   public:
    virtual scoped_refptr<StaticBitmapImage> TransferToStaticBitmapImage() = 0;
  };

  explicit XRFrameProvider(XRDevice*);

  XRSession* exclusive_session() const { return exclusive_session_; }

  void BeginExclusiveSession(XRSession*, ScriptPromiseResolver*);

  void OnExclusiveSessionEnded();

  void RequestFrame(XRSession*);

  void OnNonExclusiveVSync(double timestamp);

  void SubmitFrame(ImageSource*);
  void UpdateWebGLLayerViewports(XRWebGLLayer*);

  // VRSubmitFrameClient
  void OnSubmitFrameTransferred(bool success);
  void OnSubmitFrameRendered();

  void Dispose();

  virtual void Trace(blink::Visitor*);

 private:
  // Specifies how submitFrame should transport frame data for the presenting
  // VR device, set by ConfigurePresentationPathForDisplay().
  enum class FrameTransport {
    // Invalid default value. Must be changed to a valid choice before starting
    // presentation.
    kUninitialized,

    // Command buffer CHROMIUM_texture_mailbox. Used by the Android Surface
    // rendering path.
    kMailbox,

    // A TextureHandle as extracted from a GpuMemoryBufferHandle. Used with
    // DXGI texture handles for OpenVR on Windows.
    kTextureHandle,
  };

  // Some implementations need to synchronize submitting with the completion of
  // the previous frame, i.e. the Android surface path needs to wait to avoid
  // lost frames in the transfer surface and to avoid overstuffed buffers. The
  // strategy choice here indicates at which point in the submission process
  // it should wait. NO_WAIT means to skip this wait entirely. For example,
  // the OpenVR render pipeline doesn't overlap frames, so the previous
  // frame is already guaranteed complete.
  enum class WaitPrevStrategy {
    kUninitialized,
    kNoWait,
    kBeforeBitmap,
    kAfterBitmap,
  };

  void OnExclusiveVSync(
      device::mojom::blink::VRPosePtr,
      WTF::TimeDelta,
      int16_t frame_id,
      device::mojom::blink::VRPresentationProvider::VSyncStatus);
  void OnNonExclusivePose(device::mojom::blink::VRPosePtr);

  void ScheduleExclusiveFrame();
  void ScheduleNonExclusiveFrame();

  void OnPresentComplete(bool success);
  void OnPresentationProviderConnectionError();
  void ProcessScheduledFrame(double timestamp);

  void ConfigurePresentationPathForDisplay();
  void WaitForPreviousTransfer();
  WTF::TimeDelta WaitForPreviousRenderToFinish();

  const Member<XRDevice> device_;
  Member<XRSession> exclusive_session_;
  Member<ScriptPromiseResolver> pending_exclusive_session_resolver_;

  // Non-exclusive Sessions which have requested a frame update.
  HeapVector<Member<XRSession>> requesting_sessions_;

  mojo::Binding<device::mojom::blink::VRSubmitFrameClient>
      submit_frame_client_binding_;
  device::mojom::blink::VRPresentationProviderPtr presentation_provider_;
  device::mojom::blink::VRMagicWindowProviderPtr magic_window_provider_;
  device::mojom::blink::VRPosePtr frame_pose_;

  // Used to keep the image alive until the next frame if using
  // waitForPreviousTransferToFinish.
  scoped_refptr<Image> previous_image_;

  FrameTransport frame_transport_method_ = FrameTransport::kUninitialized;
  WaitPrevStrategy wait_for_previous_render_ = WaitPrevStrategy::kUninitialized;

  // This frame ID is VR-specific and is used to track when frames arrive at the
  // VR compositor so that it knows which poses to use, when to apply bounds
  // updates, etc.
  int16_t frame_id_ = -1;
  bool pending_exclusive_vsync_ = false;
  bool pending_non_exclusive_vsync_ = false;
  bool vsync_connection_failed_ = false;

  bool pending_submit_frame_ = false;
  bool pending_previous_frame_render_ = false;
};

}  // namespace blink

#endif  // XRFrameProvider_h
