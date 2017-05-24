// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef VRSession_h
#define VRSession_h

#include "bindings/core/v8/ScriptPromise.h"
#include "core/events/EventTarget.h"
#include "modules/vr/latest/VRSessionCreateParameters.h"
#include "platform/heap/Handle.h"
#include "platform/wtf/Forward.h"

namespace blink {

class VRDevice;
class VRLayer;

class VRSession final : public EventTargetWithInlineData,
                        public GarbageCollectedMixin {
  DEFINE_WRAPPERTYPEINFO();
  USING_GARBAGE_COLLECTED_MIXIN(VRSession);

 public:
  VRSession(VRDevice*);

  VRDevice* device() const { return device_; }
  VRSessionCreateParameters* createParameters() const {
    return create_parameters_;
  }

  double depthNear() const { return depth_near_; }
  void setDepthNear(double value) { depth_near_ = value; }
  double depthFar() const { return depth_far_; }
  void setDepthFar(double value) { depth_far_ = value; }

  VRLayer* baseLayer() const { return base_layer_; }
  void setBaseLayer(VRLayer* value);

  DEFINE_ATTRIBUTE_EVENT_LISTENER(blur);
  DEFINE_ATTRIBUTE_EVENT_LISTENER(focus);
  DEFINE_ATTRIBUTE_EVENT_LISTENER(resetpose);

  ScriptPromise createFrameOfReference(ScriptState*, const String& type);

  ScriptPromise requestVRPresentationFrame(ScriptState*);

  ScriptPromise endSession(ScriptState*);

  // EventTarget overrides.
  ExecutionContext* GetExecutionContext() const override;
  const AtomicString& InterfaceName() const override;

  DECLARE_VIRTUAL_TRACE();

 private:
  Member<VRDevice> device_;
  Member<VRSessionCreateParameters> create_parameters_;
  Member<VRLayer> base_layer_;

  double depth_near_;
  double depth_far_;
};

}  // namespace blink

#endif  // VRWebGLLayer_h
