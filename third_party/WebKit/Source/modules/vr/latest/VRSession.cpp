// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/vr/latest/VRSession.h"

#include "bindings/core/v8/ScriptPromiseResolver.h"
#include "core/dom/DOMException.h"
#include "modules/EventTargetModules.h"
#include "modules/vr/latest/VRDevice.h"
#include "modules/vr/latest/VRLayer.h"

namespace blink {

VRSession::VRSession(VRDevice* device)
    : device_(device), depth_near_(0.1), depth_far_(1000.0) {}

ExecutionContext* VRSession::GetExecutionContext() const {
  return device_->GetExecutionContext();
}

const AtomicString& VRSession::InterfaceName() const {
  return EventTargetNames::VRSession;
}

void VRSession::setBaseLayer(VRLayer* value) {
  base_layer_ = value;
}

ScriptPromise VRSession::createFrameOfReference(ScriptState* script_state,
                                                const String& type) {
  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();
  DOMException* exception =
      DOMException::Create(kNotSupportedError, "Method not implemented yet");
  resolver->Reject(exception);
  return promise;
}

ScriptPromise VRSession::requestVRPresentationFrame(ScriptState* script_state) {
  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();
  DOMException* exception =
      DOMException::Create(kNotSupportedError, "Method not implemented yet");
  resolver->Reject(exception);
  return promise;
}

ScriptPromise VRSession::endSession(ScriptState* script_state) {
  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();
  DOMException* exception =
      DOMException::Create(kNotSupportedError, "Method not implemented yet");
  resolver->Reject(exception);
  return promise;
}

DEFINE_TRACE(VRSession) {
  visitor->Trace(device_);
  visitor->Trace(create_parameters_);
  visitor->Trace(base_layer_);
}

}  // namespace blink
