// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/resource_coordinator/coordination_unit/tab_signal_generator_impl.h"

#include <utility>

#include "services/resource_coordinator/coordination_unit/coordination_unit_impl.h"
#include "services/resource_coordinator/coordination_unit/frame_coordination_unit_impl.h"
#include "services/resource_coordinator/coordination_unit/web_contents_coordination_unit_impl.h"

namespace resource_coordinator {

#define DISPATCH_TAB_SIGNAL(observers, METHOD, cu, ...)           \
  observers.ForAllPtrs([cu](mojom::TabSignalObserver* observer) { \
    observer->METHOD(cu->id(), __VA_ARGS__);                      \
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

void TabSignalGeneratorImpl::OnFramePropertyChanged(
    const FrameCoordinationUnitImpl* frame_cu,
    const mojom::PropertyType property_type,
    int64_t value) {
  if (property_type == mojom::PropertyType::kLongTaskIdle ||
      property_type == mojom::PropertyType::kNetworkIdle) {
    // Ignore when the signal doesn't come from main frame.
    if (!frame_cu->IsMainFrame())
      return;
    if (IsDoneLoading(frame_cu)) {
      auto* web_contents_cu = frame_cu->GetWebContentsCoordinationUnit();
      if (!web_contents_cu)
        return;
      DISPATCH_TAB_SIGNAL(observers_, OnEventReceived, web_contents_cu,
                          mojom::TabEvent::kDoneLoading);
    }
  }
}

void TabSignalGeneratorImpl::BindToInterface(
    resource_coordinator::mojom::TabSignalGeneratorRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

bool TabSignalGeneratorImpl::IsDoneLoading(
    const FrameCoordinationUnitImpl* frame_cu) {
  int64_t is_long_task_idle = 0;
  int64_t is_network_idle = 0;
  if (!frame_cu->GetProperty(mojom::PropertyType::kLongTaskIdle,
                             &is_long_task_idle) ||
      !frame_cu->GetProperty(mojom::PropertyType::kNetworkIdle,
                             &is_network_idle)) {
    return false;
  }
  return is_long_task_idle && is_network_idle;
}

}  // namespace resource_coordinator
