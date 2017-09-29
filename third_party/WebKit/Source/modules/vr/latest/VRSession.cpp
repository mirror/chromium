// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/vr/latest/VRSession.h"

#include "bindings/core/v8/ScriptPromiseResolver.h"
#include "core/dom/DOMException.h"
#include "core/dom/UserGestureIndicator.h"
#include "core/events/MouseEvent.h"
#include "core/frame/LocalFrame.h"
#include "modules/EventTargetModules.h"
#include "modules/vr/latest/VR.h"
#include "modules/vr/latest/VRDevice.h"
#include "modules/vr/latest/VRFrameOfReference.h"
#include "modules/vr/latest/VRFrameOfReferenceOptions.h"
#include "modules/vr/latest/VRFrameProvider.h"
#include "modules/vr/latest/VRFrameRequestCallback.h"
#include "modules/vr/latest/VRLayer.h"
#include "modules/vr/latest/VRPresentationContext.h"
#include "modules/vr/latest/VRPresentationFrame.h"
#include "modules/vr/latest/VRSessionEvent.h"
#include "modules/vr/latest/input/VRControllerInputSource.h"
#include "modules/vr/latest/input/VRGestureEvent.h"
#include "platform/wtf/AutoReset.h"

namespace blink {

namespace {

const char kSessionEnded[] = "VRSession has already ended.";

const char kUnknownFrameOfReference[] = "Unknown frame of reference type";

const double DEG_TO_RAD = M_PI / 180.0;

class VRSessionEventListener : public EventListener {
 public:
  static VRSessionEventListener* Create(VRSession* session) {
    return new VRSessionEventListener(session);
  }
  static const VRSessionEventListener* Cast(const EventListener* listener) {
    return listener->GetType() == kVRSessionEventListenerType
               ? static_cast<const VRSessionEventListener*>(listener)
               : 0;
  }

  bool operator==(const EventListener& listener) const {
    if (const VRSessionEventListener* session_event_listener =
            VRSessionEventListener::Cast(&listener))
      return session_ == session_event_listener->session_;
    return false;
  }

  DEFINE_INLINE_VIRTUAL_TRACE() {
    visitor->Trace(session_);
    EventListener::Trace(visitor);
  }

 private:
  VRSessionEventListener(VRSession* session)
      : EventListener(kVRSessionEventListenerType), session_(session) {}

  void handleEvent(ExecutionContext* execution_context, Event* event) override {
    if (event->type() == EventTypeNames::click) {
      session_->OnCanvasSelectGesture(ToMouseEvent(event));
    }
  }

  Member<VRSession> session_;
};

}  // namespace

VRSession::VRSession(VRDevice* device,
                     bool exclusive,
                     VRPresentationContext* output_context)
    : device_(device),
      exclusive_(exclusive),
      output_context_(output_context),
      callback_collection_(device->GetExecutionContext()),
      session_client_binding_(this) {
  presentation_frame_ = new VRPresentationFrame(this);
  UpdateCanvasDimensions();
  BindClickListener();
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

ScriptPromise VRSession::requestFrameOfReference(
    ScriptState* script_state,
    const String& type,
    const VRFrameOfReferenceOptions& options) {
  if (detached_) {
    return ScriptPromise::RejectWithDOMException(
        script_state, DOMException::Create(kInvalidStateError, kSessionEnded));
  }

  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();

  if (type == "headModel") {
    VRFrameOfReference* frameOfRef =
        new VRFrameOfReference(this, VRFrameOfReference::kTypeHeadModel);
    resolver->Resolve(frameOfRef);
  } else if (type == "eyeLevel") {
    VRFrameOfReference* frameOfRef =
        new VRFrameOfReference(this, VRFrameOfReference::kTypeEyeLevel);
    resolver->Resolve(frameOfRef);
  } else if (type == "stage") {
    if (!options.disableStageEmulation()) {
      VRFrameOfReference* frameOfRef =
          new VRFrameOfReference(this, VRFrameOfReference::kTypeStage);
      frameOfRef->UseEmulatedHeight(options.stageEmulationHeight());

      resolver->Resolve(frameOfRef);
    } else {
      // TODO(bajones): Support native stages using the standing transform.
      DOMException* exception = DOMException::Create(
          kNotSupportedError,
          "Non-emulated 'stage' frame of reference not yet supported");
      resolver->Reject(exception);
    }
  } else {
    DOMException* exception =
        DOMException::Create(kNotSupportedError, kUnknownFrameOfReference);
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
  // Don't allow a session to end twice.
  if (detached_) {
    return ScriptPromise::RejectWithDOMException(
        script_state, DOMException::Create(kInvalidStateError, kSessionEnded));
  }

  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();

  // TODO(bajones): If there's any work that needs to be done asynchronously on
  // session end it should be completed before this promise is resolved.

  if (device_->frameProvider()->exclusive_session() == this) {
    // Clear the device's exclusive session. (Calls ForceEnd)
    device_->frameProvider()->EndExclusiveSession();
  }

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

void VRSession::OnControllerConnected(
    device::mojom::blink::VRControllerInfoPtr controller_info) {
  HeapHashMap<String, Member<VRControllerElement>> elements;

  for (const auto& element_info : controller_info->elements) {
    LOG(ERROR) << "Adding Controller Element[" << element_info->name << "]";
    LOG(ERROR) << " - xAxis: " << element_info->hasXAxis;
    LOG(ERROR) << " - yAxis: " << element_info->hasYAxis;
    VRControllerElement* element =
        new VRControllerElement(element_info->hasXAxis, element_info->hasYAxis);

    if (element_info->initialState) {
      const device::mojom::blink::VRControllerElementStatePtr& state =
          element_info->initialState;
      element->setValue(state->value, state->pressed, state->touched);
      element->setAxes(state->xAxis, state->yAxis);
    }

    elements.insert(element_info->name, element);
  }

  VRControllerInputSource* controller = new VRControllerInputSource(
      this, controller_info->index, controller_info->gazeCursor, elements);

  controller->setHandedness(static_cast<VRControllerInputSource::Handedness>(
      controller_info->handedness));

  if (controller_info->pointerTransform.has_value()) {
    const WTF::Vector<float>& m = controller_info->pointerTransform.value();
    std::unique_ptr<TransformationMatrix> pointer_transform_matrix =
        TransformationMatrix::Create(m[0], m[1], m[2], m[3], m[4], m[5], m[6],
                                     m[7], m[8], m[9], m[10], m[11], m[12],
                                     m[13], m[14], m[15]);
    controller->set_pointer_transform_matrix(
        std::move(pointer_transform_matrix));
  }

  controllers_.push_back(controller);

  // TODO(bajones): Fire controller connected event.
}

void VRSession::OnControllerDisconnected(uint32_t index) {
  for (size_t i = 0; i < controllers_.size(); ++i) {
    if (controllers_[i]->index() == index) {
      controllers_.EraseAt(i);
      // TODO(bajones): Fire controller disconnected event.
      break;
    }
  }
}

void VRSession::OnControllerStateChange(
    WTF::Vector<device::mojom::blink::VRControllerStatePtr> controllers) {
  for (const auto& controller_state : controllers) {
    UpdateControllerState(controller_state);
  }
}

void VRSession::OnControllerStateChange(
    device::mojom::blink::VRControllerStatePtr controller_state) {
  UpdateControllerState(controller_state);
}

void VRSession::OnControllerSelectGesture(
    device::mojom::blink::VRControllerStatePtr controller_state) {
  LocalFrame* frame = device_->vr()->GetFrame();
  if (!frame)
    return;

  VRControllerInputSource* controller = UpdateControllerState(controller_state);

  if (controller) {
    std::unique_ptr<UserGestureIndicator> gesture_indicator =
        LocalFrame::CreateUserGesture(frame);

    VRGestureEvent* event = VRGestureEvent::Create(
        EventTypeNames::select, presentation_frame_, controller);
    DispatchEvent(event);
  }
}

void VRSession::OnCanvasSelectGesture(MouseEvent* event) {
  // TODO: Dispatch

  // Don't process canvas gestures if there's an active exclusive session.
  // TODO: This logic may need to change for desktop VR.
  if (device_->frameProvider()->exclusive_session()) {
    return;
  }

  HTMLCanvasElement* canvas = outputContext()->canvas();
  if (!canvas) {
    return;
  }

  // May set views_dirty_
  UpdateCanvasDimensions();
  if (views_dirty_)
    UpdateViews();

  // Get the event location relative to the canvas element.
  double element_x = event->pageX() - canvas->OffsetLeft();
  double element_y = event->pageY() - canvas->OffsetTop();

  VRInputSource* input_source =
      new VRInputSource(this, 0, kVRPointerInputSource, true);

  // Unproject the event location into a pointer matrix.
  VRView* left_view = presentation_frame_->views()[0];
  std::unique_ptr<TransformationMatrix> pointer_transform_matrix =
      left_view->unprojectPointerTransform(
          element_x, element_y, last_canvas_width_, last_canvas_height_);

  input_source->set_pointer_transform_matrix(
      std::move(pointer_transform_matrix));

  // Dispatch the gesture event.
  VRGestureEvent* gesture = VRGestureEvent::Create(
      EventTypeNames::select, presentation_frame_, input_source);
  DispatchEvent(gesture);
}

device::mojom::blink::VRSessionClientPtr VRSession::GetSessionClientPtr() {
  device::mojom::blink::VRSessionClientPtr session_client;
  session_client_binding_.Close();
  session_client_binding_.Bind(mojo::MakeRequest(&session_client));
  return session_client;
}

VRControllerInputSource* VRSession::UpdateControllerState(
    const device::mojom::blink::VRControllerStatePtr& controller_state) {
  // TODO(bajones): This could probably stand to be a lot faster
  for (size_t i = 0; i < controllers_.size(); ++i) {
    if (controllers_[i]->index() == controller_state->index) {
      VRControllerInputSource* controller = controllers_[i];
      controller->setHandedness(
          static_cast<VRControllerInputSource::Handedness>(
              controller_state->handedness));
      for (const auto& state : controller_state->elements) {
        VRControllerElement* element = controller->elements()->At(state->name);
        if (element) {
          element->setValue(state->value, state->pressed, state->touched);
          element->setAxes(state->xAxis, state->yAxis);
        }
      }

      if (controller_state->poseMatrix.has_value()) {
        const WTF::Vector<float>& m = controller_state->poseMatrix.value();
        std::unique_ptr<TransformationMatrix> pose_matrix =
            TransformationMatrix::Create(m[0], m[1], m[2], m[3], m[4], m[5],
                                         m[6], m[7], m[8], m[9], m[10], m[11],
                                         m[12], m[13], m[14], m[15]);
        controller->set_base_pose_matrix(std::move(pose_matrix));
      } else {
        controller->set_base_pose_matrix(nullptr);
      }

      return controller;
    }
  }
  return nullptr;
}

void VRSession::Dispose() {
  session_client_binding_.Close();
}

void VRSession::OnFocus() {
  if (!blurred_)
    return;

  blurred_ = false;
  DispatchEvent(VRSessionEvent::Create(EventTypeNames::focus, this));
}

void VRSession::OnBlur() {
  if (blurred_)
    return;

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

    presentation_frame_->InFrameCallback(true);
    callback_collection_.ExecuteCallbacks(presentation_frame_);
    presentation_frame_->InFrameCallback(false);

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

void VRSession::BindClickListener() {
  if (outputContext()) {
    VRSessionEventListener* listener = VRSessionEventListener::Create(this);
    outputContext()->host()->HostAddEventListener(EventTypeNames::click,
                                                  listener);
  }
}

DEFINE_TRACE(VRSession) {
  visitor->Trace(device_);
  visitor->Trace(output_context_);
  visitor->Trace(base_layer_);
  visitor->Trace(presentation_frame_);
  visitor->Trace(callback_collection_);
  visitor->Trace(controllers_);
  EventTargetWithInlineData::Trace(visitor);
}

}  // namespace blink
