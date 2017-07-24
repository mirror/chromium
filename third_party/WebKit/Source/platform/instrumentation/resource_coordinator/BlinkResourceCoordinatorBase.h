// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BlinkResourceCoordinatorBase_h
#define BlinkResourceCoordinatorBase_h

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

class PLATFORM_EXPORT BlinkResourceCoordinatorBase
    : public GarbageCollectedFinalized<BlinkResourceCoordinatorBase> {
  WTF_MAKE_NONCOPYABLE(BlinkResourceCoordinatorBase);

 public:
  static bool IsEnabled();
  virtual ~BlinkResourceCoordinatorBase();

  template <typename T>
  void SetProperty(
      const resource_coordinator::mojom::blink::PropertyType property_type,
      T value) {
    service_->SetProperty(property_type, base::MakeUnique<base::Value>(value));
  }

  DECLARE_TRACE();

 protected:
  explicit BlinkResourceCoordinatorBase(service_manager::InterfaceProvider*);
  explicit BlinkResourceCoordinatorBase(blink::InterfaceProvider*);

 private:
  resource_coordinator::mojom::blink::CoordinationUnitPtr service_;
};

}  // namespace blink

#endif  // BlinkResourceCoordinatorBase_h
