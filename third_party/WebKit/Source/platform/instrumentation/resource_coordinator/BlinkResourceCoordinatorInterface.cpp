// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/instrumentation/resource_coordinator/BlinkResourceCoordinatorInterface.h"

#include "platform/wtf/Functional.h"
#include "services/resource_coordinator/public/cpp/resource_coordinator_features.h"
#include "services/resource_coordinator/public/interfaces/coordination_unit_provider.mojom-blink.h"
#include "services/service_manager/public/cpp/interface_provider.h"

namespace blink {

namespace {

void onConnectionError() {}

}  // namespace

// static
bool BlinkResourceCoordinatorInterface::IsEnabled() {
  return base::FeatureList::IsEnabled(features::kGlobalResourceCoordinator);
}

BlinkResourceCoordinatorInterface::~BlinkResourceCoordinatorInterface() =
    default;

void BlinkResourceCoordinatorInterface::SetProperty(
    const resource_coordinator::mojom::blink::PropertyType property_type,
    std::unique_ptr<base::Value> value) {
  service_->SetProperty(property_type, std::move(value));
}

void BlinkResourceCoordinatorInterface::SetProperty(
    const resource_coordinator::mojom::blink::PropertyType property_type,
    bool value) {
  service_->SetProperty(property_type, base::MakeUnique<base::Value>(value));
}

BlinkResourceCoordinatorInterface::BlinkResourceCoordinatorInterface(
    service_manager::InterfaceProvider* interface_provider) {
  interface_provider->GetInterface(mojo::MakeRequest(&service_));
  service_.set_connection_error_handler(
      ConvertToBaseCallback(WTF::Bind(&onConnectionError)));
}

BlinkResourceCoordinatorInterface::BlinkResourceCoordinatorInterface(
    blink::InterfaceProvider* interface_provider) {
  interface_provider->GetInterface(mojo::MakeRequest(&service_));
  service_.set_connection_error_handler(
      ConvertToBaseCallback(WTF::Bind(&onConnectionError)));
}

DEFINE_TRACE(BlinkResourceCoordinatorInterface) {}

}  // namespace blink
