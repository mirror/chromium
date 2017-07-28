// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/vr/latest/VRSession.h"

#include "bindings/core/v8/ScriptPromiseResolver.h"
#include "core/dom/DOMException.h"
#include "modules/EventTargetModules.h"
#include "modules/vr/latest/VRDevice.h"
#include "modules/vr/latest/VRFrameOfReference.h"
#include "modules/vr/latest/VRFrameProvider.h"
#include "modules/vr/latest/VRFrameRequestCallback.h"
#include "modules/vr/latest/VRLayer.h"
#include "modules/vr/latest/VRPresentationFrame.h"
#include "platform/wtf/AutoReset.h"

namespace blink {

namespace {

const double DEG_TO_RAD = M_PI / 180.0;

}  // namespace

VRSession::VRSession(VRDevice* device,
                     VRSessionCreateParameters* create_parameters)
    : device_(device),
      create_parameters_(create_parameters),
      callback_collection_(device->GetExecutionContext()),
      binding_(this) {
  presentation_frame_ = new VRPresentationFrame(this);
}

void VRSession::setDepthNear(double value) {
  if (depth_near_ != value) {
    views_dirty_ = true;
    depth_near_ = value;
  }
}

void VRSession::setDepthFar(double value) {
  if (depth_far_ != value) {
    views_dirty_ = true;
    depth_far_ = value;
  }
}

ExecutionContext* VRSession::GetExecutionContext() const {
  return device_->GetExecutionContext();
}

const AtomicString& VRSession::InterfaceName() const {
  return EventTargetNames::VRSession;
}

void VRSession::setBaseLayer(VRLayer* value) {
  base_layer_ = value;
}

ScriptPromise VRSession::requestFrameOfReference(ScriptState* script_state,
                                                 const String& type) {
  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();

  if (detached_) {
    DOMException* exception =
        DOMException::Create(kNotSupportedError, "VRSession has ended");
    resolver->Reject(exception);
    return promise;
  }

  if (type == "headModel") {
    VRFrameOfReference* frameOfRef =
        new VRFrameOfReference(this, VRFrameOfReference::kTypeHeadModel);
    resolver->Resolve(frameOfRef);
  } else if (type == "eyeLevel") {
    VRFrameOfReference* frameOfRef =
        new VRFrameOfReference(this, VRFrameOfReference::kTypeEyeLevel);
    resolver->Resolve(frameOfRef);
  } else if (type == "stage") {
    DOMException* exception = DOMException::Create(
        kNotSupportedError, "'stage' frame of reference not yet supported");
    resolver->Reject(exception);
  } else {
    DOMException* exception = DOMException::Create(
        kNotSupportedError, "Unknown frame of reference type");
    resolver->Reject(exception);
  }

  return promise;
}

int VRSession::requestFrame(VRFrameRequestCallback* callback) {
  // Don't allow any new frame requests once the session is ended.
  if (detached_)
    return 0;

  int id = callback_collection_.RegisterCallback(callback);
  if (!pending_frame_) {
    // Kick off a request for a new VR frame.
    device_->frameProvider()->RequestFrame(this);
    pending_frame_ = true;
  }
  return id;
}

void VRSession::cancelFrame(int id) {
  callback_collection_.CancelCallback(id);
}

ScriptPromise VRSession::endSession(ScriptState* script_state) {
  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();

  // Don't allow a session to end twice.
  if (detached_) {
    DOMException* exception = DOMException::Create(
        kNotSupportedError, "VRSession has already ended.");
    resolver->Reject(exception);
    return promise;
  }

  if (device_->frameProvider()->exclusive_session() == this) {
    // Clear the device's exclusive session. (Calls ForceEndSession)
    device_->frameProvider()->EndExclusiveSession();
  }

  // TODO(bajones): If there's any work that needs to be done asynchronously on
  // session end it should be completed before this promise is resolved.

  resolver->Resolve();
  return promise;
}

void VRSession::ForceEndSession() {
  // TODO: If there's pending frames when we end the session discard them.
  pending_frame_ = false;

  // Detach this session from the device.
  detached_ = true;

  Dispose();
}

double VRSession::IdealFramebufferWidth() const {
  return device_->vrDisplayInfoPtr()->leftEye->renderWidth +
         device_->vrDisplayInfoPtr()->rightEye->renderWidth;
}

double VRSession::IdealFramebufferHeight() const {
  return std::max(device_->vrDisplayInfoPtr()->leftEye->renderHeight,
                  device_->vrDisplayInfoPtr()->rightEye->renderHeight);
}

void VRSession::OnSubmitFrameTransferred() {}
void VRSession::OnSubmitFrameRendered() {}

void VRSession::Dispose() {
  binding_.Close();
}

void VRSession::OnFrame(
    std::unique_ptr<TransformationMatrix> base_pose_matrix) {
  DVLOG(2) << __FUNCTION__;
  // Don't process any outstanding frames once the session is ended.
  if (detached_)
    return;

  // Don't allow frames to be processed if there's no layers attached to the
  // session. That would allow tracking with no associated visuals.
  if (!base_layer_)
    return;

  if (views_dirty_)
    UpdateViews();

  presentation_frame_->UpdateBasePose(std::move(base_pose_matrix));

  if (pending_frame_) {
    pending_frame_ = false;

    base_layer_->OnFrameStart();

    // Resolve the queued requestFrame callbacks. All VR rendering will happen
    // within these calls. resolving_frame_ will be true for the duration of the
    // callbacks.
    AutoReset<bool> resolving(&resolving_frame_, true);
    callback_collection_.ExecuteCallbacks(presentation_frame_);

    // TODO(bajones): Check for dirty layers and submit their respective images.
    base_layer_->OnFrameEnd();
  }
}

void VRSession::UpdateViews() {
  VRView* left_view = presentation_frame_->views()[0];
  const device::mojom::blink::VREyeParametersPtr& left_eye =
      device_->vrDisplayInfoPtr()->leftEye;

  const device::mojom::blink::VRFieldOfViewPtr& left_fov =
      left_eye->fieldOfView;

  left_view->UpdateProjectionMatrix(
      left_fov->upDegrees * DEG_TO_RAD, left_fov->downDegrees * DEG_TO_RAD,
      left_fov->leftDegrees * DEG_TO_RAD, left_fov->rightDegrees * DEG_TO_RAD,
      depth_near_, depth_far_);

  left_view->UpdateOffset(left_eye->offset[0], left_eye->offset[1],
                          left_eye->offset[2]);

  if (isExclusive()) {
    VRView* right_view = presentation_frame_->views()[1];
    const device::mojom::blink::VREyeParametersPtr& right_eye =
        device_->vrDisplayInfoPtr()->rightEye;

    const device::mojom::blink::VRFieldOfViewPtr& right_fov =
        right_eye->fieldOfView;

    right_view->UpdateProjectionMatrix(
        right_fov->upDegrees * DEG_TO_RAD, right_fov->downDegrees * DEG_TO_RAD,
        right_fov->leftDegrees * DEG_TO_RAD,
        right_fov->rightDegrees * DEG_TO_RAD, depth_near_, depth_far_);

    right_view->UpdateOffset(right_eye->offset[0], right_eye->offset[1],
                             right_eye->offset[2]);
  }

  views_dirty_ = false;
}

DEFINE_TRACE(VRSession) {
  visitor->Trace(device_);
  visitor->Trace(create_parameters_);
  visitor->Trace(base_layer_);
  visitor->Trace(presentation_frame_);
  visitor->Trace(callback_collection_);
}

}  // namespace blink
