// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef VRDevice_h
#define VRDevice_h

#include "bindings/core/v8/ScriptPromise.h"
#include "bindings/modules/v8/WebGLRenderingContextOrWebGL2RenderingContext.h"
#include "core/events/EventTarget.h"
#include "device/vr/vr_service.mojom-blink.h"
#include "modules/vr/latest/VRSessionCreationOptions.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "platform/heap/Handle.h"
#include "platform/wtf/Forward.h"
#include "platform/wtf/text/WTFString.h"

namespace blink {

class VR;
class VRFrameProvider;

class VRDevice final : public EventTargetWithInlineData,
                       public GarbageCollectedMixin,
                       public device::mojom::blink::VRDisplayClient {
  DEFINE_WRAPPERTYPEINFO();
  USING_GARBAGE_COLLECTED_MIXIN(VRDevice);

 public:
  VRDevice(VR*,
           device::mojom::blink::VRDisplayPtr,
           device::mojom::blink::VRDisplayClientRequest);

  const String& deviceName() const { return device_name_; }
  void setDeviceName(const String& value) { device_name_ = value; }
  bool isExternal() const { return is_external_; }
  void setIsExternal(bool value) { is_external_ = value; }

  DEFINE_ATTRIBUTE_EVENT_LISTENER(activate);
  DEFINE_ATTRIBUTE_EVENT_LISTENER(deactivate);

  ScriptPromise supportsSession(ScriptState*,
                                const VRSessionCreationOptions&) const;
  ScriptPromise requestSession(ScriptState*, const VRSessionCreationOptions&);

  ScriptPromise ensureContextCompatibility(
      ScriptState*,
      const WebGLRenderingContextOrWebGL2RenderingContext&) const;

  // EventTarget overrides.
  ExecutionContext* GetExecutionContext() const override;
  const AtomicString& InterfaceName() const override;

  // VRDisplayClient
  void OnChanged(device::mojom::blink::VRDisplayInfoPtr) override;
  void OnExitPresent() override;
  void OnBlur() override;
  void OnFocus() override;
  void OnActivate(device::mojom::blink::VRDisplayEventReason,
                  OnActivateCallback on_handled) override;
  void OnDeactivate(device::mojom::blink::VRDisplayEventReason) override;

  VRFrameProvider* frameProvider();

  VR* vr() const { return vr_; }

  void Dispose();

  const device::mojom::blink::VRDisplayPtr& vrDisplayPtr() const {
    return display_;
  }
  const device::mojom::blink::VRDisplayInfoPtr& vrDisplayInfoPtr() const {
    return display_info_;
  }

  void SetVRDisplayInfo(device::mojom::blink::VRDisplayInfoPtr);

  DECLARE_VIRTUAL_TRACE();

 private:
  Member<VR> vr_;
  Member<VRFrameProvider> frame_provider_;
  String device_name_;
  bool is_external_;

  device::mojom::blink::VRDisplayPtr display_;
  device::mojom::blink::VRDisplayInfoPtr display_info_;

  mojo::Binding<device::mojom::blink::VRDisplayClient> display_client_binding_;
};

using VRDeviceVector = HeapVector<Member<VRDevice>>;

}  // namespace blink

#endif  // VRDevice_h
