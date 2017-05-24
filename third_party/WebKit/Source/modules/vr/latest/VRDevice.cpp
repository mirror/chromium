// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/vr/latest/VRDevice.h"

#include "bindings/core/v8/ScriptPromiseResolver.h"
#include "core/dom/DOMException.h"
#include "modules/EventTargetModules.h"
#include "modules/vr/latest/VR.h"
#include "modules/vr/latest/VRSession.h"
#include "modules/webgl/WebGLRenderingContext.h"

namespace blink {

VRDevice::VRDevice(VR* vr,
                   device::mojom::blink::VRDisplayPtr device,
                   device::mojom::blink::VRDisplayClientRequest client_request)
    : vr_(vr),
      device_(std::move(device)),
      submit_frame_client_binding_(this),
      device_client_binding_(this, std::move(client_request)) {}

ExecutionContext* VRDevice::GetExecutionContext() const {
  return vr_->GetExecutionContext();
}

const AtomicString& VRDevice::InterfaceName() const {
  return EventTargetNames::VRDevice;
}

ScriptPromise VRDevice::supportsSession(
    ScriptState* script_state,
    const VRSessionCreateParametersInit& parameters) const {
  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();
  DOMException* exception =
      DOMException::Create(kNotSupportedError, "Method not implemented yet");
  resolver->Reject(exception);
  return promise;
}

ScriptPromise VRDevice::requestSession(
    ScriptState* script_state,
    const VRSessionCreateParametersInit& parameters) {
  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();
  DOMException* exception =
      DOMException::Create(kNotSupportedError, "Method not implemented yet");
  resolver->Reject(exception);
  return promise;
}

ScriptPromise VRDevice::ensureContextCompatibility(
    ScriptState* script_state,
    const WebGLRenderingContextOrWebGL2RenderingContext& context) const {
  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();
  DOMException* exception =
      DOMException::Create(kNotSupportedError, "Method not implemented yet");
  resolver->Reject(exception);
  return promise;
}

void VRDevice::OnChanged(device::mojom::blink::VRDisplayInfoPtr) {}
void VRDevice::OnExitPresent() {}
void VRDevice::OnBlur() {}
void VRDevice::OnFocus() {}
void VRDevice::OnActivate(device::mojom::blink::VRDisplayEventReason,
                          const OnActivateCallback& on_handled) {}
void VRDevice::OnDeactivate(device::mojom::blink::VRDisplayEventReason) {}

void VRDevice::OnSubmitFrameTransferred() {}
void VRDevice::OnSubmitFrameRendered() {}

void VRDevice::Dispose() {
  device_client_binding_.Close();
  // vr_v_sync_provider_.reset();
}

DEFINE_TRACE(VRDevice) {
  visitor->Trace(vr_);
  EventTargetWithInlineData::Trace(visitor);
}

}  // namespace blink
