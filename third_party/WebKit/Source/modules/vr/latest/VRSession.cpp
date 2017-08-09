// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/vr/latest/VRSession.h"

#include "bindings/core/v8/ScriptPromiseResolver.h"
#include "core/dom/DOMException.h"
#include "core/frame/LocalFrame.h"
#include "modules/EventTargetModules.h"
#include "modules/vr/latest/VR.h"
#include "modules/vr/latest/VRDevice.h"
#include "modules/vr/latest/VRFrameOfReference.h"
#include "modules/vr/latest/VRFrameProvider.h"
#include "modules/vr/latest/VRFrameRequestCallback.h"
#include "modules/vr/latest/VRLayer.h"
#include "modules/vr/latest/VRPresentationContext.h"
#include "modules/vr/latest/VRPresentationFrame.h"
#include "modules/vr/latest/VRSessionEvent.h"
#include "platform/wtf/AutoReset.h"

namespace blink {

namespace {

const double DEG_TO_RAD = M_PI / 180.0;

}  // namespace

VRSession::VRSession(VRDevice* device,
                     bool exclusive,
                     VRPresentationContext* output_context)
    : device_(device),
      exclusive_(exclusive),
      output_context_(output_context),
      callback_collection_(device->GetExecutionContext()),
      binding_(this) {
  presentation_frame_ = new VRPresentationFrame(this);
  UpdateCanvasDimensions();
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

ScriptPromise VRSession::end(ScriptState* script_state) {
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
    // Clear the device's exclusive session. (Calls ForceEnd)
    device_->frameProvider()->EndExclusiveSession();
  }

  // TODO(bajones): If there's any work that needs to be done asynchronously on
  // session end it should be completed before this promise is resolved.

  resolver->Resolve();
  return promise;
}

void VRSession::ForceEnd() {
  // TODO: If there's pending frames when we end the session discard them.
  pending_frame_ = false;

  // Detach this session from the device.
  detached_ = true;

  DispatchEvent(VRSessionEvent::Create(EventTypeNames::end, this));

  Dispose();
}

double VRSession::IdealFramebufferWidth() const {
  if (exclusive_) {
    return device_->vrDisplayInfoPtr()->leftEye->renderWidth +
           device_->vrDisplayInfoPtr()->rightEye->renderWidth;
  }
  double devicePixelRatio = 1.0;
  LocalFrame* frame = device_->vr()->GetFrame();
  if (frame) {
    devicePixelRatio = frame->DevicePixelRatio();
  }
  return last_canvas_width_ * devicePixelRatio;
}

double VRSession::IdealFramebufferHeight() const {
  if (exclusive_) {
    return std::max(device_->vrDisplayInfoPtr()->leftEye->renderHeight,
                    device_->vrDisplayInfoPtr()->rightEye->renderHeight);
  }
  double devicePixelRatio = 1.0;
  LocalFrame* frame = device_->vr()->GetFrame();
  if (frame) {
    devicePixelRatio = frame->DevicePixelRatio();
  }
  return last_canvas_height_ * devicePixelRatio;
}

void VRSession::OnSubmitFrameTransferred() {}
void VRSession::OnSubmitFrameRendered() {}

void VRSession::Dispose() {
  binding_.Close();
}

void VRSession::OnFocus() {
  if (!blurred_)
    return;

  DVLOG(1) << __FUNCTION__;
  blurred_ = false;
  DispatchEvent(VRSessionEvent::Create(EventTypeNames::focus, this));
}

void VRSession::OnBlur() {
  if (blurred_)
    return;

  DVLOG(1) << __FUNCTION__;
  blurred_ = true;
  DispatchEvent(VRSessionEvent::Create(EventTypeNames::blur, this));
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

  if (!exclusive_) {
    // May set views_dirty_
    UpdateCanvasDimensions();
  }

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

  if (exclusive_) {
    // In exclusive mode the projection and view matrices must be aligned
    // with the device's physical optics.
    const device::mojom::blink::VRFieldOfViewPtr& left_fov =
        left_eye->fieldOfView;

    left_view->UpdateProjectionMatrixFromFoV(
        left_fov->upDegrees * DEG_TO_RAD, left_fov->downDegrees * DEG_TO_RAD,
        left_fov->leftDegrees * DEG_TO_RAD, left_fov->rightDegrees * DEG_TO_RAD,
        depth_near_, depth_far_);

    left_view->UpdateOffset(left_eye->offset[0], left_eye->offset[1],
                            left_eye->offset[2]);

    VRView* right_view = presentation_frame_->views()[1];
    const device::mojom::blink::VREyeParametersPtr& right_eye =
        device_->vrDisplayInfoPtr()->rightEye;

    const device::mojom::blink::VRFieldOfViewPtr& right_fov =
        right_eye->fieldOfView;

    right_view->UpdateProjectionMatrixFromFoV(
        right_fov->upDegrees * DEG_TO_RAD, right_fov->downDegrees * DEG_TO_RAD,
        right_fov->leftDegrees * DEG_TO_RAD,
        right_fov->rightDegrees * DEG_TO_RAD, depth_near_, depth_far_);

    right_view->UpdateOffset(right_eye->offset[0], right_eye->offset[1],
                             right_eye->offset[2]);
  } else {
    float aspect = 1.0f;
    if (last_canvas_width_ && last_canvas_height_) {
      aspect = static_cast<float>(last_canvas_width_) /
               static_cast<float>(last_canvas_height_);
    }

    // In non-exclusive mode the projection matrix must be aligned with the
    // output canvas dimensions and the view matrix should not have any IPD
    // offset.
    left_view->UpdateProjectionMatrixFromAspect(
        M_PI * 0.4f,  // TODO(bajones): Not this! Maybe user configurable?
        aspect, depth_near_, depth_far_);

    left_view->UpdateOffset(0, 0, 0);
  }

  views_dirty_ = false;
}

void VRSession::UpdateCanvasDimensions() {
  if (outputContext()) {
    HTMLCanvasElement* canvas = outputContext()->canvas();
    if (!canvas) {
      return;
    }
    // TODO(bajones): It would be nice if there was a way to listen for size
    // changes rather than poll them every frame.
    int new_width = canvas->OffsetWidth();
    int new_height = canvas->OffsetHeight();
    if (new_width != last_canvas_width_ || new_height != last_canvas_height_) {
      views_dirty_ = true;
      last_canvas_width_ = new_width;
      last_canvas_height_ = new_height;

      if (base_layer_) {
        base_layer_->OnResize();
      }
    }
  }
}

DEFINE_TRACE(VRSession) {
  visitor->Trace(device_);
  visitor->Trace(output_context_);
  visitor->Trace(base_layer_);
  visitor->Trace(presentation_frame_);
  visitor->Trace(callback_collection_);
}

}  // namespace blink
