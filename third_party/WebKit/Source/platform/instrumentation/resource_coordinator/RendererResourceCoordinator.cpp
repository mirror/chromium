// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/instrumentation/resource_coordinator/RendererResourceCoordinator.h"

#include "public/platform/Platform.h"

namespace blink {

namespace {

RendererResourceCoordinator* g_renderer_resource_coordinator = nullptr;

}  // namespace

// static
void RendererResourceCoordinator::Initialize() {
  blink::Platform* platform = Platform::Current();
  DCHECK(platform);
  g_renderer_resource_coordinator = new RendererResourceCoordinator(
      platform->GetConnector(), platform->GetNavigationServiceName());
}

// static
void RendererResourceCoordinator::
    SetCurrentRendererResourceCoordinatorForTesting(
        RendererResourceCoordinator* renderer_resource_coordinator) {
  g_renderer_resource_coordinator = renderer_resource_coordinator;
}

// static
RendererResourceCoordinator& RendererResourceCoordinator::Get() {
  DCHECK(g_renderer_resource_coordinator);
  return *g_renderer_resource_coordinator;
}

RendererResourceCoordinator::RendererResourceCoordinator(
    service_manager::Connector* connector,
    const std::string& navigation_service_name)
    : BlinkResourceCoordinatorBase(connector, navigation_service_name) {}

RendererResourceCoordinator::RendererResourceCoordinator() {}

RendererResourceCoordinator::~RendererResourceCoordinator() = default;

}  // namespace blink
