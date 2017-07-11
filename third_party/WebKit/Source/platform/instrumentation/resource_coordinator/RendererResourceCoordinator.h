// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef RendererResourceCoordinator_h
#define RendererResourceCoordinator_h

#include "platform/heap/Visitor.h"
#include "services/resource_coordinator/public/interfaces/coordination_unit.mojom-blink.h"

namespace blink {

class InterfaceProvider;

class PLATFORM_EXPORT RendererResourceCoordinator final {
  WTF_MAKE_NONCOPYABLE(RendererResourceCoordinator);

 public:
  static bool IsEnabled();
  static RendererResourceCoordinator* Get();
  virtual ~RendererResourceCoordinator();

  void SetProperty(const resource_coordinator::mojom::blink::PropertyType&,
                   std::unique_ptr<base::Value>);

  DECLARE_TRACE();

 private:
  explicit RendererResourceCoordinator(InterfaceProvider*);

  resource_coordinator::mojom::blink::CoordinationUnitPtr service_;
};

}  // namespace blink

#endif  // RendererResourceCoordinator_h
