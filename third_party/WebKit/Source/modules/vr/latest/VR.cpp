// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/vr/latest/VR.h"

#include "bindings/core/v8/ScriptPromiseResolver.h"
#include "core/dom/DOMException.h"
#include "core/dom/Document.h"
#include "core/frame/LocalFrame.h"
#include "modules/EventTargetModules.h"
#include "modules/vr/latest/VRDevice.h"
#include "modules/vr/latest/VRDeviceEvent.h"
#include "public/platform/InterfaceProvider.h"
#include "services/service_manager/public/cpp/interface_provider.h"

namespace blink {

// Success and failure callbacks for getDevices.
class VRDevicesCallback final : public WebVRDevicesCallback {
  USING_FAST_MALLOC(VRDevicesCallback);

 public:
  VRDevicesCallback(ScriptPromiseResolver* resolver) : resolver_(resolver) {}
  ~VRDevicesCallback() override {}

  void OnSuccess(VRDeviceVector devices) override {
    resolver_->Resolve(devices);
  }
  void OnError() override { resolver_->Reject(); }

 private:
  Persistent<ScriptPromiseResolver> resolver_;
};

VR::VR(LocalFrame& frame)
    : ContextLifecycleObserver(frame.GetDocument()),
      devices_synced_(false),
      binding_(this) {
  frame.GetInterfaceProvider().GetInterface(mojo::MakeRequest(&service_));
  service_.set_connection_error_handler(
      ConvertToBaseCallback(WTF::Bind(&VR::Dispose, WrapWeakPersistent(this))));

  device::mojom::blink::VRServiceClientPtr client;
  binding_.Bind(mojo::MakeRequest(&client));
  service_->SetClient(std::move(client),
                      ConvertToBaseCallback(WTF::Bind(&VR::OnDevicesSynced,
                                                      WrapPersistent(this))));
}

ExecutionContext* VR::GetExecutionContext() const {
  return ContextLifecycleObserver::GetExecutionContext();
}

const AtomicString& VR::InterfaceName() const {
  return EventTargetNames::VR;
}

ScriptPromise VR::getDevices(ScriptState* script_state) {
  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();

  // If we've previously synced the VRDevices or no longer have a valid service
  // connection just return the current list. In the case of the service being
  // disconnected this will be an empty array.
  if (!service_ || devices_synced_) {
    resolver->Resolve(devices_);
    return promise;
  }

  // Otherwise we're still waiting for the full list of devices to be populated
  // so queue up the promise for resolution when onDevicesSynced is called.
  pending_devices_callbacks_.push_back(
      WTF::MakeUnique<VRDevicesCallback>(resolver));

  return promise;
}

// Each time a new VRDevice is connected we'll recieve a VRDisplayPtr for it
// here. Upon calling SetClient in the constructor we should receive one call
// for each VRDevice that was already connected at the time.
void VR::OnDisplayConnected(
    device::mojom::blink::VRDisplayPtr display,
    device::mojom::blink::VRDisplayClientRequest client_request,
    device::mojom::blink::VRDisplayInfoPtr display_info) {
  VRDevice* vr_device =
      new VRDevice(this, std::move(display), std::move(client_request));
  vr_device->SetVRDisplayInfo(std::move(display_info));

  /*vr_device->Update(display_info);
  vr_device->OnConnected();
  vr_device->FocusChanged();*/
  devices_.push_back(vr_device);

  DispatchEvent(
      VRDeviceEvent::Create(EventTypeNames::deviceconnect, vr_device));

  if (devices_.size() == number_of_synced_devices_) {
    devices_synced_ = true;
    OnGetDevices();
  }
}

// Called when the VRService has called OnDevicesConnected for all active
// VRDevices.
void VR::OnDevicesSynced(unsigned number_of_devices) {
  number_of_synced_devices_ = number_of_devices;
  if (number_of_synced_devices_ == devices_.size()) {
    devices_synced_ = true;
    OnGetDevices();
  }
}

void VR::OnGetDevices() {
  while (!pending_devices_callbacks_.IsEmpty()) {
    std::unique_ptr<VRDevicesCallback> callback =
        pending_devices_callbacks_.TakeFirst();
    callback->OnSuccess(devices_);
  }
}

void VR::ContextDestroyed(ExecutionContext*) {
  Dispose();
}

void VR::Dispose() {
  // If the document context was destroyed, shut down the client connection
  // and never call the mojo service again.
  service_.reset();
  binding_.Close();

  // Shutdown all devices' message pipe
  for (const auto& device : devices_)
    device->Dispose();

  devices_.clear();

  // Ensure that any outstanding getDevices promises are resolved.
  OnGetDevices();
}

DEFINE_TRACE(VR) {
  visitor->Trace(devices_);
  ContextLifecycleObserver::Trace(visitor);
}

}  // namespace blink
