// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/vr/latest/VRDevice.h"

#include "bindings/core/v8/ScriptPromiseResolver.h"
#include "core/dom/DOMException.h"
#include "modules/EventTargetModules.h"
#include "modules/vr/latest/VR.h"
#include "modules/vr/latest/VRFrameProvider.h"
#include "modules/vr/latest/VRSession.h"
#include "modules/webgl/WebGLRenderingContext.h"

namespace blink {

VRDevice::VRDevice(VR* vr,
                   device::mojom::blink::VRDisplayPtr display,
                   device::mojom::blink::VRDisplayClientRequest client_request)
    : vr_(vr),
      display_(std::move(display)),
      display_client_binding_(this, std::move(client_request)) {}

ExecutionContext* VRDevice::GetExecutionContext() const {
  return vr_->GetExecutionContext();
}

const AtomicString& VRDevice::InterfaceName() const {
  return EventTargetNames::VRDevice;
}

ScriptPromise VRDevice::supportsSession(
    ScriptState* script_state,
    const VRSessionCreationOptions& options) const {
  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();
  DOMException* exception =
      DOMException::Create(kNotSupportedError, "Method not implemented yet");
  resolver->Reject(exception);
  return promise;
}

ScriptPromise VRDevice::requestSession(
    ScriptState* script_state,
    const VRSessionCreationOptions& options) {
  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();

  if (options.exclusive() && frameProvider()->exclusive_session()) {
    DOMException* exception = DOMException::Create(
        kNotSupportedError,
        "VRDevice already has an active, exclusive session");
    resolver->Reject(exception);
  }

  // TODO(bajones): Needs a *lot* more validation here, to determine if the
  // requested session type can be supported.

  VRPresentationContext* output_context = nullptr;
  if (options.hasOutputContext()) {
    output_context = options.outputContext();
  }

  VRSession* session = new VRSession(this, options.exclusive(), output_context);

  if (options.exclusive()) {
    frameProvider()->BeginExclusiveSession(session, resolver);
  } else {
    resolver->Resolve(session);
  }

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
void VRDevice::OnBlur() {
  // TODO: Forward this on to the sessions
}
void VRDevice::OnFocus() {
  // TODO: Forward this on to the sessions
}
void VRDevice::OnActivate(device::mojom::blink::VRDisplayEventReason,
                          OnActivateCallback on_handled) {}
void VRDevice::OnDeactivate(device::mojom::blink::VRDisplayEventReason) {}

VRFrameProvider* VRDevice::frameProvider() {
  if (!frame_provider_)
    frame_provider_ = new VRFrameProvider(this);

  return frame_provider_;
}

void VRDevice::Dispose() {
  display_client_binding_.Close();
  if (frame_provider_)
    frame_provider_->Dispose();
}

void VRDevice::SetVRDisplayInfo(
    device::mojom::blink::VRDisplayInfoPtr display_info) {
  display_info_ = std::move(display_info);
  setDeviceName(display_info_->displayName);
  setIsExternal(display_info_->capabilities->hasExternalDisplay);
}

DEFINE_TRACE(VRDevice) {
  visitor->Trace(vr_);
  visitor->Trace(frame_provider_);
  EventTargetWithInlineData::Trace(visitor);
}

}  // namespace blink
