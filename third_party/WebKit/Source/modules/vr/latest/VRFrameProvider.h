// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef VRFrameProvider_h
#define VRFrameProvider_h

#include "device/vr/vr_service.mojom-blink.h"
#include "gpu/command_buffer/common/mailbox_holder.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "platform/bindings/ScriptWrappable.h"
#include "platform/bindings/TraceWrapperMember.h"
#include "platform/heap/Handle.h"
#include "platform/wtf/Forward.h"

namespace blink {

class ScriptPromiseResolver;
class VRDevice;
class VRSession;
class VRWebGLLayer;

// This class manages requesting and dispatching frame updates, which includes
// pose information for a given VRDevice.
class VRFrameProvider final : public GarbageCollectedFinalized<VRFrameProvider>,
                              public device::mojom::blink::VRSubmitFrameClient,
                              public TraceWrapperBase {
 public:
  VRFrameProvider(VRDevice*);

  VRSession* exclusive_session() const { return exclusive_session_; }

  void BeginExclusiveSession(VRSession*, ScriptPromiseResolver*);
  void EndExclusiveSession();

  void RequestFrame(VRSession*);

  void Dispose();

  void OnNonExclusiveVSync(double timestamp);

  void SubmitFrame(gpu::MailboxHolder);

  void UpdateWebGLLayerViewports(VRWebGLLayer*);

  // VRSubmitFrameClient
  void OnSubmitFrameTransferred();
  void OnSubmitFrameRendered();

  DECLARE_VIRTUAL_TRACE();

  DECLARE_TRACE_WRAPPERS();

 private:
  void OnExclusiveVSync(
      device::mojom::blink::VRPosePtr,
      WTF::TimeDelta,
      int16_t frame_id,
      device::mojom::blink::VRPresentationProvider::VSyncStatus);
  void OnNonExclusivePose(device::mojom::blink::VRPosePtr);

  void ScheduleNonExclusiveFrame();

  void OnPresentComplete(bool success);
  void OnPresentationProviderConnectionError();
  void ProcessScheduledFrame(double timestamp);

  const Member<VRDevice> device_;
  TraceWrapperMember<VRSession> exclusive_session_;
  Member<ScriptPromiseResolver> pending_exclusive_session_resolver_;

  // Non-exclusive Sessions which have requested a frame update.
  HeapVector<TraceWrapperMember<VRSession>> requesting_sessions_;

  mojo::Binding<device::mojom::blink::VRSubmitFrameClient>
      submit_frame_client_binding_;
  device::mojom::blink::VRPresentationProviderPtr presentation_provider_;
  device::mojom::blink::VRPosePtr frame_pose_;

  // This frame ID is VR-specific and is used to track when frames arrive at the
  // VR compositor so that it knows which poses to use, when to apply bounds
  // updates, etc.
  int16_t frame_id_ = -1;
  bool pending_exclusive_vsync_ = false;
  bool pending_non_exclusive_vsync_ = false;
  bool vsync_connection_failed_ = false;
  double timebase_ = -1;

  bool pending_submit_frame_ = false;
  bool pending_previous_frame_render_ = false;
};

}  // namespace blink

#endif  // VRFrameProvider_h
