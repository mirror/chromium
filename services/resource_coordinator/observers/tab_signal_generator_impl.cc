// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/resource_coordinator/observers/tab_signal_generator_impl.h"

#include <utility>

#include "services/resource_coordinator/coordination_unit/coordination_unit_base.h"
#include "services/resource_coordinator/coordination_unit/frame_coordination_unit_impl.h"
#include "services/resource_coordinator/coordination_unit/page_coordination_unit_impl.h"
#include "services/service_manager/public/cpp/bind_source_info.h"

namespace resource_coordinator {

#define DISPATCH_TAB_SIGNAL(observers, METHOD, cu, ...)          \
  observers.ForAllPtrs([&](mojom::TabSignalObserver* observer) { \
    observer->METHOD(cu->id(), __VA_ARGS__);                     \
  });

TabSignalGeneratorImpl::TabSignalGeneratorImpl() = default;

TabSignalGeneratorImpl::~TabSignalGeneratorImpl() = default;

void TabSignalGeneratorImpl::AddObserver(mojom::TabSignalObserverPtr observer) {
  observers_.AddPtr(std::move(observer));
}

bool TabSignalGeneratorImpl::ShouldObserve(
    const CoordinationUnitBase* coordination_unit) {
  auto coordination_unit_type = coordination_unit->id().type;
  return coordination_unit_type == CoordinationUnitType::kPage ||
         coordination_unit_type == CoordinationUnitType::kFrame;
}

void TabSignalGeneratorImpl::OnFramePropertyChanged(
    const FrameCoordinationUnitImpl* frame_cu,
    const mojom::PropertyType property_type,
    int64_t value) {
  if (property_type == mojom::PropertyType::kNetworkAlmostIdle ||
      property_type == mojom::PropertyType::kLowMainThreadLoad) {
    // Ignore when the signal doesn't come from main frame.
    if (!value || !frame_cu->IsMainFrame())
      return;
    if (IsAlmostIdle(frame_cu)) {
      auto* page_cu = frame_cu->GetPageCoordinationUnit();
      if (!page_cu)
        return;
      DISPATCH_TAB_SIGNAL(observers_, OnEventReceived, page_cu,
                          mojom::TabEvent::kAlmostIdle);
    }
  }
}

void TabSignalGeneratorImpl::OnPagePropertyChanged(
    const PageCoordinationUnitImpl* page_cu,
    const mojom::PropertyType property_type,
    int64_t value) {
  if (property_type == mojom::PropertyType::kExpectedTaskQueueingDuration) {
    DISPATCH_TAB_SIGNAL(observers_, OnPropertyChanged, page_cu, property_type,
                        value);
  }
}

void TabSignalGeneratorImpl::BindToInterface(
    resource_coordinator::mojom::TabSignalGeneratorRequest request,
    const service_manager::BindSourceInfo& source_info) {
  bindings_.AddBinding(this, std::move(request));
}

bool TabSignalGeneratorImpl::IsAlmostIdle(
    const FrameCoordinationUnitImpl* frame_cu) const {
  int64_t is_main_thread_load_low = 0;
  int64_t is_network_almost_idle = 0;
  if (!frame_cu->GetProperty(mojom::PropertyType::kLowMainThreadLoad,
                             &is_main_thread_load_low) ||
      !frame_cu->GetProperty(mojom::PropertyType::kNetworkAlmostIdle,
                             &is_network_almost_idle)) {
    return false;
  }
  return is_main_thread_load_low && is_network_almost_idle;
}

}  // namespace resource_coordinator
