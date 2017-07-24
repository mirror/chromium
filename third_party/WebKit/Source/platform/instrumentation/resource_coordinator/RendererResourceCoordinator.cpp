// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/instrumentation/resource_coordinator/RendererResourceCoordinator.h"

#include "public/platform/Platform.h"

namespace blink {

namespace {

static Persistent<RendererResourceCoordinator>*
    g_renderer_resource_coordinator = nullptr;

}  // namespace

// static
RendererResourceCoordinator* RendererResourceCoordinator::Get() {
  if (!g_renderer_resource_coordinator) {
    g_renderer_resource_coordinator =
        new Persistent<RendererResourceCoordinator>(
            new RendererResourceCoordinator(
                Platform::Current()->GetInterfaceProvider()));
  }
  return g_renderer_resource_coordinator->Get();
}

RendererResourceCoordinator::RendererResourceCoordinator(
    blink::InterfaceProvider* interface_provider)
    : BlinkResourceCoordinatorBase(interface_provider) {}

RendererResourceCoordinator::~RendererResourceCoordinator() = default;

}  // namespace blink
