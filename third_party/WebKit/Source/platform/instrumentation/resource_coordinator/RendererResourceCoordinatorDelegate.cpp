// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/instrumentation/resource_coordinator/RendererResourceCoordinatorDelegate.h"

#include "platform/instrumentation/resource_coordinator/RendererResourceCoordinator.h"
#include "services/resource_coordinator/public/cpp/resource_coordinator_features.h"

namespace blink {

// static
bool RendererResourceCoordinatorDelegate::IsEnabled() {
  return resource_coordinator::IsResourceCoordinatorEnabled();
}

// static
void RendererResourceCoordinatorDelegate::SetExpectedTaskQueueingDuration(
    int64_t eqt) {
  RendererResourceCoordinator::Get().SetProperty(
      resource_coordinator::mojom::PropertyType::kExpectedTaskQueueingDuration,
      eqt);
}

RendererResourceCoordinatorDelegate::RendererResourceCoordinatorDelegate() {}

}  // namespace blink
