// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/instrumentation/resource_coordinator/RendererResourceCoordinator.h"

#include "public/platform/InterfaceProvider.h"
#include "public/platform/Platform.h"
#include "services/resource_coordinator/public/cpp/resource_coordinator_features.h"
#include "services/resource_coordinator/public/interfaces/coordination_unit_provider.mojom-blink.h"

namespace blink {

namespace {

void OnConnectionError() {}

RendererResourceCoordinator* g_renderer_resource_coordinator = nullptr;

}  // namespace

// static
bool RendererResourceCoordinator::IsEnabled() {
  return base::FeatureList::IsEnabled(features::kGlobalResourceCoordinator);
}

// static
RendererResourceCoordinator* RendererResourceCoordinator::Get() {
  if (!g_renderer_resource_coordinator) {
    g_renderer_resource_coordinator = new RendererResourceCoordinator(
        Platform::Current()->GetInterfaceProvider());
  }
  return g_renderer_resource_coordinator;
}

RendererResourceCoordinator::RendererResourceCoordinator(
    InterfaceProvider* interface_provider) {
  interface_provider->GetInterface(mojo::MakeRequest(&service_));

  service_.set_connection_error_handler(
      ConvertToBaseCallback(WTF::Bind(&OnConnectionError)));
}

RendererResourceCoordinator::~RendererResourceCoordinator() = default;

void RendererResourceCoordinator::SetProperty(
    const resource_coordinator::mojom::blink::PropertyType& property_type,
    std::unique_ptr<base::Value> value) {
  service_->SetProperty(property_type, std::move(value));
}

}  // namespace blink
