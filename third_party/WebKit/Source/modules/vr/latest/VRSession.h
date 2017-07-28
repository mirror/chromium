// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef VRSession_h
#define VRSession_h

#include "bindings/core/v8/ScriptPromise.h"
#include "core/events/EventTarget.h"
#include "device/vr/vr_service.mojom-blink.h"
#include "modules/vr/latest/VRFrameRequestCallbackCollection.h"
#include "modules/vr/latest/VRSessionCreateParameters.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "platform/heap/Handle.h"
#include "platform/transforms/TransformationMatrix.h"
#include "platform/wtf/Forward.h"

namespace blink {

class VRDevice;
class VRFrameRequestCallback;
class VRLayer;
class VRPresentationContext;
class VRPresentationFrame;

class VRSession final : public EventTargetWithInlineData,
                        public GarbageCollectedMixin,
                        public device::mojom::blink::VRSubmitFrameClient {
  DEFINE_WRAPPERTYPEINFO();
  USING_GARBAGE_COLLECTED_MIXIN(VRSession);

 public:
  VRSession(VRDevice*, VRSessionCreateParameters*);

  VRDevice* device() const { return device_; }
  VRSessionCreateParameters* createParameters() const {
    return create_parameters_;
  }

  double depthNear() const { return depth_near_; }
  void setDepthNear(double value);
  double depthFar() const { return depth_far_; }
  void setDepthFar(double value);

  VRLayer* baseLayer() const { return base_layer_; }
  void setBaseLayer(VRLayer* value);

  DEFINE_ATTRIBUTE_EVENT_LISTENER(blur);
  DEFINE_ATTRIBUTE_EVENT_LISTENER(focus);
  DEFINE_ATTRIBUTE_EVENT_LISTENER(resetpose);

  ScriptPromise requestFrameOfReference(ScriptState*, const String& type);

  int requestFrame(VRFrameRequestCallback*);
  void cancelFrame(int id);

  ScriptPromise endSession(ScriptState*);
  void ForceEndSession();

  double IdealFramebufferScale() const { return 1.0; }
  double IdealFramebufferWidth() const;
  double IdealFramebufferHeight() const;

  // EventTarget overrides.
  ExecutionContext* GetExecutionContext() const override;
  const AtomicString& InterfaceName() const override;

  // VRSubmitFrameClient
  void OnSubmitFrameTransferred() override;
  void OnSubmitFrameRendered() override;

  void Dispose();

  void OnFrame(std::unique_ptr<TransformationMatrix>);
  void UpdateViews();

  bool isExclusive() const { return create_parameters_->exclusive(); }
  VRPresentationContext* outputContext() const {
    return create_parameters_->outputContext();
  }

  DECLARE_VIRTUAL_TRACE();

 private:
  const Member<VRDevice> device_;
  const Member<VRSessionCreateParameters> create_parameters_;
  Member<VRLayer> base_layer_;

  Member<VRPresentationFrame> presentation_frame_;

  VRFrameRequestCallbackCollection callback_collection_;

  double depth_near_ = 0.1;
  double depth_far_ = 1000.0;
  bool views_dirty_ = true;
  bool pending_frame_ = false;
  bool resolving_frame_ = false;

  bool detached_ = false;

  mojo::Binding<device::mojom::blink::VRSubmitFrameClient> binding_;
};

}  // namespace blink

#endif  // VRWebGLLayer_h
