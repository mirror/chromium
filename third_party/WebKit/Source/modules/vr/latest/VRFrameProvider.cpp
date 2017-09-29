// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/vr/latest/VRFrameProvider.h"

#include "bindings/core/v8/ScriptPromiseResolver.h"
#include "core/dom/DOMException.h"
#include "core/dom/Document.h"
#include "core/dom/FrameRequestCallbackCollection.h"
#include "core/frame/LocalFrame.h"
#include "modules/vr/latest/VR.h"
#include "modules/vr/latest/VRDevice.h"
#include "modules/vr/latest/VRSession.h"
#include "modules/vr/latest/VRViewport.h"
#include "modules/vr/latest/VRWebGLLayer.h"
#include "platform/WebTaskRunner.h"
#include "platform/instrumentation/tracing/TraceEvent.h"
#include "platform/transforms/TransformationMatrix.h"
#include "platform/wtf/Time.h"
#include "public/platform/Platform.h"

namespace blink {

namespace {

class VRFrameProviderRequestCallback
    : public FrameRequestCallbackCollection::FrameCallback {
 public:
  explicit VRFrameProviderRequestCallback(VRFrameProvider* frame_provider)
      : frame_provider_(frame_provider) {}
  ~VRFrameProviderRequestCallback() override {}
  void Invoke(double high_res_time_ms) override {
    frame_provider_->OnNonExclusiveVSync(high_res_time_ms / 1000.0);
  }

  DEFINE_INLINE_VIRTUAL_TRACE() {
    visitor->Trace(frame_provider_);

    FrameRequestCallbackCollection::FrameCallback::Trace(visitor);
  }

  Member<VRFrameProvider> frame_provider_;
};

std::unique_ptr<TransformationMatrix> getPoseMatrix(
    const device::mojom::blink::VRPosePtr& pose) {
  if (!pose)
    return nullptr;

  std::unique_ptr<TransformationMatrix> pose_matrix =
      TransformationMatrix::Create();

  TransformationMatrix::DecomposedType decomp;

  memset(&decomp, 0, sizeof(decomp));
  decomp.perspective_w = 1;
  decomp.scale_x = 1;
  decomp.scale_y = 1;
  decomp.scale_z = 1;

  if (pose->orientation) {
    decomp.quaternion_x = -pose->orientation.value()[0];
    decomp.quaternion_y = -pose->orientation.value()[1];
    decomp.quaternion_z = -pose->orientation.value()[2];
    decomp.quaternion_w = pose->orientation.value()[3];
  } else {
    decomp.quaternion_w = 1.0;
  }

  if (pose->position) {
    decomp.translate_x = pose->position.value()[0];
    decomp.translate_y = pose->position.value()[1];
    decomp.translate_z = pose->position.value()[2];
  }

  pose_matrix->Recompose(decomp);

  return pose_matrix;
}

}  // namespace

VRFrameProvider::VRFrameProvider(VRDevice* device)
    : device_(device), submit_frame_client_binding_(this) {}

void VRFrameProvider::BeginExclusiveSession(VRSession* session,
                                            ScriptPromiseResolver* resolver) {
  // Make sure the session is indeed an exclusive one.
  DCHECK(session && session->exclusive());

  // Ensure we can only have one exclusive session at a time.
  DCHECK(!exclusive_session_);

  exclusive_session_ = session;

  pending_exclusive_session_resolver_ = resolver;

  // Establish the connection with the VSyncProvider if needed.
  if (!presentation_provider_.is_bound()) {
    device::mojom::blink::VRSubmitFrameClientPtr submit_frame_client;
    submit_frame_client_binding_.Close();
    submit_frame_client_binding_.Bind(mojo::MakeRequest(&submit_frame_client));

    device_->vrDisplayPtr()->RequestPresent(
        std::move(submit_frame_client),
        mojo::MakeRequest(&presentation_provider_),
        ConvertToBaseCallback(WTF::Bind(&VRFrameProvider::OnPresentComplete,
                                        WrapPersistent(this))));

    presentation_provider_.set_connection_error_handler(ConvertToBaseCallback(
        WTF::Bind(&VRFrameProvider::OnPresentationProviderConnectionError,
                  WrapWeakPersistent(this))));
  }
}

void VRFrameProvider::OnPresentComplete(bool success) {
  if (success) {
    presentation_provider_->SetSessionClient(
        exclusive_session_->GetSessionClientPtr());
    pending_exclusive_session_resolver_->Resolve(exclusive_session_);
  } else {
    EndExclusiveSession();

    if (pending_exclusive_session_resolver_) {
      DOMException* exception = DOMException::Create(
          kNotAllowedError, "Request for exclusive VRSession was denied.");
      pending_exclusive_session_resolver_->Reject(exception);
    }
  }

  pending_exclusive_session_resolver_ = nullptr;
}

void VRFrameProvider::OnPresentationProviderConnectionError() {
  if (pending_exclusive_session_resolver_) {
    DOMException* exception = DOMException::Create(
        kNotAllowedError,
        "Error occured while requesting exclusive VRSession.");
    pending_exclusive_session_resolver_->Reject(exception);
    pending_exclusive_session_resolver_ = nullptr;
  }

  presentation_provider_.reset();
  if (vsync_connection_failed_)
    return;
  EndExclusiveSession();
  vsync_connection_failed_ = true;
}

void VRFrameProvider::EndExclusiveSession() {
  if (!exclusive_session_)
    return;

  device_->vrDisplayPtr()->ExitPresent();

  exclusive_session_->ForceEnd();

  exclusive_session_ = nullptr;
  pending_exclusive_vsync_ = false;
  frame_id_ = -1;

  if (presentation_provider_.is_bound()) {
    presentation_provider_.reset();
  }

  // When we no longer have an active exclusive session schedule all the
  // outstanding frames that were requested while the exclusive session was
  // active.
  if (requesting_sessions_.size() > 0)
    ScheduleNonExclusiveFrame();
}

// Schedule a session to be notified when the next VR frame is available.
void VRFrameProvider::RequestFrame(VRSession* session) {
  DVLOG(2) << __FUNCTION__;

  // If a previous session has already requested a frame don't fire off another
  // request. All requests will be fullfilled at once when OnVSync is called.
  if (session->exclusive() && !pending_exclusive_vsync_) {
    // FIXME(bajones): Occasionally we schedule a frame here immediately after
    // beginning an exlusive session and OnExclusiveVSync simply never gets
    // called. Not sure why, but it has the effect of transitioning the screen
    // into VR mode and then leaving it black.
    pending_exclusive_vsync_ = true;
    presentation_provider_->GetVSync(ConvertToBaseCallback(WTF::Bind(
        &VRFrameProvider::OnExclusiveVSync, WrapWeakPersistent(this))));
  } else {
    requesting_sessions_.push_back(session);

    // If there's an active exclusive session save the request but suppress
    // processing it until the exclusive session is no longer active.
    if (exclusive_session_)
      return;

    ScheduleNonExclusiveFrame();
  }
}

void VRFrameProvider::ScheduleNonExclusiveFrame() {
  if (pending_non_exclusive_vsync_)
    return;

  LocalFrame* frame = device_->vr()->GetFrame();
  if (!frame)
    return;

  Document* doc = frame->GetDocument();
  if (!doc)
    return;

  pending_non_exclusive_vsync_ = true;

  device_->vrDisplayPtr()->GetNextMagicWindowPose(
      ConvertToBaseCallback(WTF::Bind(&VRFrameProvider::OnNonExclusivePose,
                                      WrapWeakPersistent(this))));
  doc->RequestAnimationFrame(new VRFrameProviderRequestCallback(this));
}

void VRFrameProvider::OnExclusiveVSync(
    device::mojom::blink::VRPosePtr pose,
    WTF::TimeDelta time_delta,
    int16_t frame_id,
    device::mojom::blink::VRPresentationProvider::VSyncStatus status) {
  DVLOG(2) << __FUNCTION__;
  vsync_connection_failed_ = false;
  switch (status) {
    case device::mojom::blink::VRPresentationProvider::VSyncStatus::SUCCESS:
      break;
    case device::mojom::blink::VRPresentationProvider::VSyncStatus::CLOSING:
      return;
  }

  // We may have lost the exclusive session since the last VSync request.
  if (!exclusive_session_) {
    return;
  }

  // Ensure a consistent timebase with document rAF.
  if (timebase_ < 0) {
    timebase_ = WTF::MonotonicallyIncreasingTime() - time_delta.InSecondsF();
  }

  frame_pose_ = std::move(pose);
  frame_id_ = frame_id;
  pending_exclusive_vsync_ = false;

  // Post a task to handle scheduled animations after the current
  // execution context finishes, so that we yield to non-mojo tasks in
  // between frames. Executing mojo tasks back to back within the same
  // execution context caused extreme input delay due to processing
  // multiple frames without yielding, see crbug.com/701444. I suspect
  // this is due to WaitForIncomingMethodCall receiving the OnVSync
  // but queueing it for immediate execution since it doesn't match
  // the interface being waited on.
  Platform::Current()->CurrentThread()->GetWebTaskRunner()->PostTask(
      BLINK_FROM_HERE,
      WTF::Bind(&VRFrameProvider::ProcessScheduledFrame,
                WrapWeakPersistent(this), timebase_ + time_delta.InSecondsF()));
}

void VRFrameProvider::OnNonExclusiveVSync(double timestamp) {
  DVLOG(2) << __FUNCTION__;

  pending_non_exclusive_vsync_ = false;

  // Suppress non-exclusive vsyncs when there's an exclusive session active.
  if (exclusive_session_)
    return;

  Platform::Current()->CurrentThread()->GetWebTaskRunner()->PostTask(
      BLINK_FROM_HERE, WTF::Bind(&VRFrameProvider::ProcessScheduledFrame,
                                 WrapWeakPersistent(this), timestamp));
}

void VRFrameProvider::OnNonExclusivePose(device::mojom::blink::VRPosePtr pose) {
  frame_pose_ = std::move(pose);
}

void VRFrameProvider::ProcessScheduledFrame(double timestamp) {
  DVLOG(2) << __FUNCTION__;

  TRACE_EVENT1("gpu", "VRFrameProvider::ProcessScheduledFrame", "frame",
               frame_id_);

  if (exclusive_session_) {
    // If there's an exclusive session active only process it's frame.
    std::unique_ptr<TransformationMatrix> pose_matrix =
        getPoseMatrix(frame_pose_);
    exclusive_session_->OnFrame(std::move(pose_matrix));
  } else {
    // In the process of fulfilling the frame requests for each session they are
    // extremely likely to request another frame. Work off of a separate list
    // from the requests to prevent infinite loops.
    HeapVector<TraceWrapperMember<VRSession>> processing_sessions;
    swap(requesting_sessions_, processing_sessions);

    // Inform sessions with a pending request of the new frame
    for (unsigned i = 0; i < processing_sessions.size(); ++i) {
      VRSession* session = processing_sessions.at(i).Get();
      std::unique_ptr<TransformationMatrix> pose_matrix =
          getPoseMatrix(frame_pose_);
      session->OnFrame(std::move(pose_matrix));
    }
  }

  // For GVR, we shut down normal vsync processing during VR presentation.
  // Trigger any callbacks on window.rAF manually so that they run after
  // completing the vrDisplay.rAF processing.
  /*if (is_presenting_ && !capabilities_->hasExternalDisplay()) {
    Platform::Current()->CurrentThread()->GetWebTaskRunner()->PostTask(
        BLINK_FROM_HERE, WTF::Bind(&VRDisplay::ProcessScheduledWindowAnimations,
                                   WrapWeakPersistent(this), timestamp));
  }*/
}

void VRFrameProvider::SubmitFrame(gpu::MailboxHolder mailbox) {
  // Invalid mailbox
  if (!mailbox.texture_target)
    return;

  if (!presentation_provider_)
    return;

  // There's two types of synchronization needed for submitting frames:
  //
  // - Before submitting, need to wait for the previous frame to be
  //   pulled off the transfer surface to avoid lost frames. This
  //   is currently a compile-time option, normally we always want
  //   to defer this wait to increase parallelism.
  //
  // - After submitting, need to wait for the mailbox to be consumed,
  //   and the image object must remain alive during this time.
  //   We keep a reference to the image so that we can defer this
  //   wait. Here, we wait for the previous transfer to complete.
  {
    TRACE_EVENT0("gpu", "VRFrameProvider::waitForPreviousTransferToFinish");
    while (pending_submit_frame_) {
      if (!submit_frame_client_binding_.WaitForIncomingMethodCall()) {
        DLOG(ERROR) << "Failed to receive SubmitFrame response";
        break;
      }
    }
  }

  // TODO: Would be nice to have some work done in here.

  // Wait for the previous render to finish, to avoid losing frames in the
  // Android Surface / GLConsumer pair. TODO(klausw): make this tunable?
  // Other devices may have different preferences. Do this step as late
  // as possible before SubmitFrame to ensure we can do as much work as
  // possible in parallel with the previous frame's rendering.
  {
    TRACE_EVENT0("gpu", "VRFrameProvider::waitForPreviousRenderToFinish");
    while (pending_previous_frame_render_) {
      if (!submit_frame_client_binding_.WaitForIncomingMethodCall()) {
        DLOG(ERROR) << "Failed to receive SubmitFrame response";
        break;
      }
    }
  }

  pending_previous_frame_render_ = true;
  pending_submit_frame_ = true;

  TRACE_EVENT_BEGIN0("gpu", "VRFrameProvider::SubmitFrame");
  presentation_provider_->SubmitFrame(frame_id_, mailbox);
  TRACE_EVENT_END0("gpu", "VRFrameProvider::SubmitFrame");

  // Reset our frame id, since anything we'd want to do (resizing/etc) can
  // no-longer happen to this frame.
  frame_id_ = -1;
}

// TODO(bajones): This only works because we're restricted to a single layer at
// the moment. Will need an overhaul when we get more robust layering support.
void VRFrameProvider::UpdateWebGLLayerViewports(VRWebGLLayer* layer) {
  if (layer->session() != exclusive_session_)
    return;

  VRViewport* left = layer->GetViewport(VRView::kEyeLeft);
  VRViewport* right = layer->GetViewport(VRView::kEyeRight);
  float width = layer->framebufferWidth();
  float height = layer->framebufferHeight();

  WebFloatRect left_coords(
      static_cast<float>(left->x()) / width,
      static_cast<float>(height - (left->y() + left->height())) / height,
      static_cast<float>(left->width()) / width,
      static_cast<float>(left->height()) / height);
  WebFloatRect right_coords(
      static_cast<float>(right->x()) / width,
      static_cast<float>(height - (right->y() + right->height())) / height,
      static_cast<float>(right->width()) / width,
      static_cast<float>(right->height()) / height);

  presentation_provider_->UpdateLayerBounds(
      frame_id_, left_coords, right_coords, WebSize(width, height));
}

void VRFrameProvider::OnSubmitFrameTransferred() {
  DVLOG(3) << __FUNCTION__;
  pending_submit_frame_ = false;
}

void VRFrameProvider::OnSubmitFrameRendered() {
  DVLOG(3) << __FUNCTION__;
  pending_previous_frame_render_ = false;
}

void VRFrameProvider::Dispose() {
  presentation_provider_.reset();
  // TODO(bajones): Do something for outstanding frame requests?
}

DEFINE_TRACE(VRFrameProvider) {
  visitor->Trace(device_);
  visitor->Trace(pending_exclusive_session_resolver_);
  visitor->Trace(exclusive_session_);
  visitor->Trace(requesting_sessions_);
}

DEFINE_TRACE_WRAPPERS(VRFrameProvider) {
  visitor->TraceWrappers(exclusive_session_);

  for (auto session : requesting_sessions_)
    visitor->TraceWrappers(session);
}

}  // namespace blink
