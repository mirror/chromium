// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/instrumentation/resource_coordinator/BlinkResourceCoordinatorBase.h"

#include "platform/wtf/Functional.h"
#include "public/platform/InterfaceProvider.h"
#include "services/resource_coordinator/public/cpp/resource_coordinator_features.h"
#include "services/resource_coordinator/public/interfaces/coordination_unit_provider.mojom-blink.h"
#include "services/service_manager/public/cpp/interface_provider.h"

namespace blink {

namespace {

void onConnectionError() {}

}  // namespace

// static
bool BlinkResourceCoordinatorBase::IsEnabled() {
  return resource_coordinator::IsResourceCoordinatorEnabled();
}

BlinkResourceCoordinatorBase::~BlinkResourceCoordinatorBase() = default;

BlinkResourceCoordinatorBase::BlinkResourceCoordinatorBase(
    service_manager::InterfaceProvider* interface_provider) {
  interface_provider->GetInterface(mojo::MakeRequest(&service_));
  service_.set_connection_error_handler(
      ConvertToBaseCallback(WTF::Bind(&onConnectionError)));
}

BlinkResourceCoordinatorBase::BlinkResourceCoordinatorBase(
    blink::InterfaceProvider* interface_provider) {
  interface_provider->GetInterface(mojo::MakeRequest(&service_));
  service_.set_connection_error_handler(
      ConvertToBaseCallback(WTF::Bind(&onConnectionError)));
}

DEFINE_TRACE(BlinkResourceCoordinatorBase) {}

}  // namespace blink
