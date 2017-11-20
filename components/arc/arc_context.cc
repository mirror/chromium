// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/arc/arc_context.h"

#include "components/arc/arc_service_manager.h"

namespace arc {

ArcContext::ArcContext(content::BrowserContext* context)
    : browser_context_(context) {}

ArcContext::~ArcContext() = default;

// static
ArcContext* ArcContext::FromBrowserContext(content::BrowserContext* context) {
  auto* arc_service_manager = arc::ArcServiceManager::Get();
  return arc_service_manager ? arc_service_manager->context() : nullptr;
}

}  // namespace arc
