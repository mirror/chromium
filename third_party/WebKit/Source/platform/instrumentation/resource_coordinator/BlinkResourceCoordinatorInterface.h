// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BlinkResourceCoordinatorInterface_h
#define BlinkResourceCoordinatorInterface_h

#include <memory>

#include "platform/PlatformExport.h"
#include "platform/heap/GarbageCollected.h"
#include "platform/heap/Heap.h"
#include "platform/heap/TraceTraits.h"
#include "platform/heap/Visitor.h"
#include "platform/wtf/Noncopyable.h"
#include "public/platform/InterfaceProvider.h"
#include "services/resource_coordinator/public/interfaces/coordination_unit.mojom-blink.h"

namespace service_manager {
class InterfaceProvider;
}

namespace blink {

class PLATFORM_EXPORT BlinkResourceCoordinatorInterface
    : public GarbageCollectedFinalized<BlinkResourceCoordinatorInterface> {
  WTF_MAKE_NONCOPYABLE(BlinkResourceCoordinatorInterface);

 public:
  static bool IsEnabled();
  virtual ~BlinkResourceCoordinatorInterface();
  void SetProperty(const resource_coordinator::mojom::blink::PropertyType,
                   std::unique_ptr<base::Value>);
  void SetProperty(const resource_coordinator::mojom::blink::PropertyType,
                   bool);

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
