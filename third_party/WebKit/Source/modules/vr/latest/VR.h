// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef VR_h
#define VR_h

#include "bindings/core/v8/ScriptPromise.h"
#include "core/dom/ContextLifecycleObserver.h"
#include "core/events/EventTarget.h"
#include "device/vr/vr_service.mojom-blink.h"
#include "modules/vr/latest/VRDevice.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "platform/heap/Handle.h"
#include "platform/wtf/Forward.h"
#include "public/platform/WebCallbacks.h"

namespace blink {

class VRDevicesCallback;

using WebVRDevicesCallback = WebCallbacks<VRDeviceVector, void>;

class VR final : public EventTargetWithInlineData,
                 public ContextLifecycleObserver,
                 public device::mojom::blink::VRServiceClient {
  DEFINE_WRAPPERTYPEINFO();
  USING_GARBAGE_COLLECTED_MIXIN(VR);

 public:
  static VR* Create(LocalFrame& frame) { return new VR(frame); }

  DEFINE_ATTRIBUTE_EVENT_LISTENER(deviceconnect);
  DEFINE_ATTRIBUTE_EVENT_LISTENER(devicedisconnect);

  ScriptPromise getDevices(ScriptState*);

  // VRServiceClient overrides.
  void OnDisplayConnected(device::mojom::blink::VRDisplayPtr,
                          device::mojom::blink::VRDisplayClientRequest,
                          device::mojom::blink::VRDisplayInfoPtr) override;

  // EventTarget overrides.
  ExecutionContext* GetExecutionContext() const override;
  const AtomicString& InterfaceName() const override;

  // ContextLifecycleObserver overrides.
  void ContextDestroyed(ExecutionContext*) override;

  void Dispose();

  DECLARE_VIRTUAL_TRACE();

 private:
  explicit VR(LocalFrame& frame);

  void OnDevicesSynced(unsigned);
  void OnGetDevices();

  bool devices_synced_;
  unsigned number_of_synced_devices_;

  VRDeviceVector devices_;
  Deque<std::unique_ptr<VRDevicesCallback>> pending_devices_callbacks_;
  device::mojom::blink::VRServicePtr service_;
  mojo::Binding<device::mojom::blink::VRServiceClient> binding_;
};

}  // namespace blink

#endif  // VR_h
