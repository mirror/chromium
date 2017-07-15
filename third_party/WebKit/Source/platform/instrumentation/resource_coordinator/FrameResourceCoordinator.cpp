// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/instrumentation/resource_coordinator/FrameResourceCoordinator.h"

namespace blink {

// static
BlinkResourceCoordinatorInterface* FrameResourceCoordinator::Create(
    service_manager::InterfaceProvider* interface_provider) {
  return new FrameResourceCoordinator(interface_provider);
}

FrameResourceCoordinator::FrameResourceCoordinator(
    service_manager::InterfaceProvider* interface_provider)
    : BlinkResourceCoordinatorInterface(interface_provider) {}

FrameResourceCoordinator::~FrameResourceCoordinator() = default;

}  // namespace blink
