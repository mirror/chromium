// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef VRSession_h
#define VRSession_h

#include "bindings/core/v8/ScriptPromise.h"
#include "core/dom/events/EventTarget.h"
#include "device/vr/vr_service.mojom-blink.h"
#include "modules/vr/latest/VRFrameRequestCallbackCollection.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "platform/heap/Handle.h"
#include "platform/transforms/TransformationMatrix.h"
#include "platform/wtf/Forward.h"

namespace blink {

class MouseEvent;
class VRControllerInputSource;
class VRDevice;
class VRFrameOfReferenceOptions;
class VRFrameRequestCallback;
class VRLayer;
class VRPresentationContext;
class VRPresentationFrame;

class VRSession final : public EventTargetWithInlineData,
                        public GarbageCollectedMixin,
                        public device::mojom::blink::VRSessionClient {
  DEFINE_WRAPPERTYPEINFO();
  USING_GARBAGE_COLLECTED_MIXIN(VRSession);

 public:
  VRSession(VRDevice*, bool exclusive, VRPresentationContext* output_context);

  VRDevice* device() const { return device_; }
  bool exclusive() const { return exclusive_; }
  VRPresentationContext* outputContext() const { return output_context_; }

  double depthNear() const { return depth_near_; }
  void setDepthNear(double value);
  double depthFar() const { return depth_far_; }
  void setDepthFar(double value);

  VRLayer* baseLayer() const { return base_layer_; }
  void setBaseLayer(VRLayer* value);

  DEFINE_ATTRIBUTE_EVENT_LISTENER(blur);
  DEFINE_ATTRIBUTE_EVENT_LISTENER(focus);
  DEFINE_ATTRIBUTE_EVENT_LISTENER(resetpose);
  DEFINE_ATTRIBUTE_EVENT_LISTENER(end);

  DEFINE_ATTRIBUTE_EVENT_LISTENER(select);

  ScriptPromise requestFrameOfReference(ScriptState*,
                                        const String& type,
                                        const VRFrameOfReferenceOptions&);

  int requestFrame(VRFrameRequestCallback*);
  void cancelFrame(int id);

  const HeapVector<Member<VRControllerInputSource>>& getControllers() const {
    return controllers_;
  }

  ScriptPromise end(ScriptState*);
  void ForceEnd();

  double IdealFramebufferScale() const { return 1.0; }
  double IdealFramebufferWidth() const;
  double IdealFramebufferHeight() const;

  // EventTarget overrides.
  ExecutionContext* GetExecutionContext() const override;
  const AtomicString& InterfaceName() const override;

  // VRSessionClient
  void OnControllerConnected(
      device::mojom::blink::VRControllerInfoPtr) override;
  void OnControllerDisconnected(uint32_t index) override;
  void OnControllerStateChange(
      WTF::Vector<device::mojom::blink::VRControllerStatePtr>);
  void OnControllerStateChange(
      device::mojom::blink::VRControllerStatePtr) override;
  void OnControllerSelectGesture(
      device::mojom::blink::VRControllerStatePtr) override;

  void OnCanvasSelectGesture(MouseEvent*);

  device::mojom::blink::VRSessionClientPtr GetSessionClientPtr();

  void Dispose();

  void OnFocus();
  void OnBlur();
  void OnFrame(std::unique_ptr<TransformationMatrix>);
  void UpdateViews();
  void UpdateCanvasDimensions();

  DECLARE_VIRTUAL_TRACE();

 private:
  VRControllerInputSource* UpdateControllerState(
      const device::mojom::blink::VRControllerStatePtr&);
  void BindClickListener();

  const Member<VRDevice> device_;
  const bool exclusive_;
  const Member<VRPresentationContext> output_context_;
  Member<VRLayer> base_layer_;

  Member<VRPresentationFrame> presentation_frame_;

  VRFrameRequestCallbackCollection callback_collection_;

  HeapVector<Member<VRControllerInputSource>> controllers_;

  double depth_near_ = 0.1;
  double depth_far_ = 1000.0;
  bool views_dirty_ = true;
  bool pending_frame_ = false;
  bool resolving_frame_ = false;
  bool blurred_ = false;

  bool detached_ = false;

  // Only used for non-exclusive sessions to track if the output canvas
  // dimensions have changed from the previous frame.
  int last_canvas_width_ = 1;
  int last_canvas_height_ = 1;

  mojo::Binding<device::mojom::blink::VRSessionClient> session_client_binding_;
};

}  // namespace blink

#endif  // VRWebGLLayer_h
