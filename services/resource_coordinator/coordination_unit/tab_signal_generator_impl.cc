// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/resource_coordinator/coordination_unit/tab_signal_generator_impl.h"

#include "base/memory/ptr_util.h"
#include "services/resource_coordinator/coordination_unit/coordination_unit_impl.h"
#include "services/resource_coordinator/coordination_unit/frame_coordination_unit_impl.h"
#include "services/resource_coordinator/coordination_unit/web_contents_coordination_unit_impl.h"

namespace resource_coordinator {

#define DISPATCH_TAB_SIGNAL(observers, METHOD, cu, ...)           \
  observers.ForAllPtrs([cu](mojom::TabSignalObserver* observer) { \
    observer->METHOD(cu->id(), __VA_ARGS__);                      \
  });

#define DISPATCH_TAB_PROPERTY_CHANGED(observers, cu, type, value)          \
  observers.ForAllPtrs(                                                    \
      [cu, &type, &value](mojom::TabSignalObserver* observer) {            \
        observer->OnPropertyChanged(cu->id(), type,                        \
                                    base::MakeUnique<base::Value>(value)); \
      });

TabSignalGeneratorImpl::TabSignalGeneratorImpl() = default;

TabSignalGeneratorImpl::~TabSignalGeneratorImpl() = default;

void TabSignalGeneratorImpl::AddObserver(mojom::TabSignalObserverPtr observer) {
  observers_.AddPtr(std::move(observer));
}

bool TabSignalGeneratorImpl::ShouldObserve(
    const CoordinationUnitImpl* coordination_unit) {
  auto coordination_unit_type = coordination_unit->id().type;
  return coordination_unit_type == CoordinationUnitType::kWebContents ||
         coordination_unit_type == CoordinationUnitType::kFrame;
}

void TabSignalGeneratorImpl::OnPropertyChanged(
    const CoordinationUnitImpl* coordination_unit,
    const mojom::PropertyType property_type,
    const base::Value& value) {
  switch (coordination_unit->id().type) {
    case CoordinationUnitType::kFrame:
      OnFramePropertyChanged(
          CoordinationUnitImpl::ToFrameCoordinationUnit(coordination_unit),
          property_type, value);
      break;
    case CoordinationUnitType::kWebContents:
      OnWebContentsPropertyChanged(
          CoordinationUnitImpl::ToWebContentsCoordinationUnit(
              coordination_unit),
          property_type, value);
      break;
    case CoordinationUnitType::kInvalidType:
    case CoordinationUnitType::kNavigation:
    case CoordinationUnitType::kProcess:
      break;
  }
}

void TabSignalGeneratorImpl::BindToInterface(
    resource_coordinator::mojom::TabSignalGeneratorRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

void TabSignalGeneratorImpl::OnFramePropertyChanged(
    const FrameCoordinationUnitImpl* coordination_unit,
    const mojom::PropertyType property_type,
    const base::Value& value) {
  if (property_type == mojom::PropertyType::kNetworkIdle) {
    // Ignore when the signal doesn't come from main frame.
    if (!coordination_unit->IsMainFrame())
      return;
    // TODO(lpy) Combine CPU usage or long task idleness signal.
    for (auto* parent : coordination_unit->parents()) {
      if (parent->id().type != CoordinationUnitType::kWebContents)
        continue;
      DISPATCH_TAB_SIGNAL(observers_, OnEventReceived, parent,
                          mojom::TabEvent::kDoneLoading);
      break;
    }
  }
}

void TabSignalGeneratorImpl::OnWebContentsPropertyChanged(
    const WebContentsCoordinationUnitImpl* coordination_unit,
    const mojom::PropertyType property_type,
    const base::Value& value) {
  DISPATCH_TAB_PROPERTY_CHANGED(observers_, coordination_unit, property_type,
                                value);
}

}  // namespace resource_coordinator
