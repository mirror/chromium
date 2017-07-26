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
  g_renderer_resource_coordinator = new RendererResourceCoordinator(
      Platform::Current()->GetInterfaceProvider());
}

// static
RendererResourceCoordinator& RendererResourceCoordinator::Get() {
  // blink::Platform must be initialized before using this class.
  DCHECK(blink::Platform::Current());
  return *g_renderer_resource_coordinator;
}

RendererResourceCoordinator::RendererResourceCoordinator(
    blink::InterfaceProvider* interface_provider)
    : BlinkResourceCoordinatorBase(interface_provider) {}

RendererResourceCoordinator::~RendererResourceCoordinator() = default;

}  // namespace blink
