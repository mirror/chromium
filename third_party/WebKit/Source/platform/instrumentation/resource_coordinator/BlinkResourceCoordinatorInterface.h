// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BlinkResourceCoordinatorInterface_h
#define BlinkResourceCoordinatorInterface_h

#include <memory>

#include "platform/PlatformExport.h"
#include "platform/heap/GarbageCollected.h"
#include "platform/heap/TraceTraits.h"
#include "platform/wtf/Noncopyable.h"
#include "services/resource_coordinator/public/interfaces/coordination_unit.mojom-blink.h"

namespace service_manager {
class InterfaceProvider;
}

namespace blink {

class InterfaceProvider;

class PLATFORM_EXPORT BlinkResourceCoordinatorInterface
    : public GarbageCollectedFinalized<BlinkResourceCoordinatorInterface> {
  WTF_MAKE_NONCOPYABLE(BlinkResourceCoordinatorInterface);

 public:
  static bool IsEnabled();
  virtual ~BlinkResourceCoordinatorInterface();

  template <typename T>
  void SetProperty(
      const resource_coordinator::mojom::blink::PropertyType property_type,
      T value) {
    service_->SetProperty(property_type, base::MakeUnique<base::Value>(value));
  }

  DECLARE_TRACE();

 protected:
  explicit BlinkResourceCoordinatorInterface(
      service_manager::InterfaceProvider*);
  explicit BlinkResourceCoordinatorInterface(blink::InterfaceProvider*);

 private:
  resource_coordinator::mojom::blink::CoordinationUnitPtr service_;
};

}  // namespace blink

#endif  // BlinkResourceCoordinatorInterface_h
