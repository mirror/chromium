// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/arc/accessibility/arc_accessibility_helper_bridge_factory.h"

#include "base/memory/singleton.h"
#include "chrome/browser/chromeos/arc/accessibility/arc_accessibility_helper_bridge.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/app_list/arc/arc_app_list_prefs_factory.h"
#include "components/arc/arc_service_manager.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

namespace arc {

// static
ArcAccessibilityHelperBridge*
ArcAccessibilityHelperBridgeFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<ArcAccessibilityHelperBridge*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

// static
ArcAccessibilityHelperBridgeFactory*
ArcAccessibilityHelperBridgeFactory::GetInstance() {
  return base::Singleton<ArcAccessibilityHelperBridgeFactory>::get();
}

ArcAccessibilityHelperBridgeFactory::ArcAccessibilityHelperBridgeFactory()
    : BrowserContextKeyedServiceFactory(
          "ArcAccessibilityHelperBridgeFactory",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(ArcAppListPrefsFactory::GetInstance());
}

ArcAccessibilityHelperBridgeFactory::~ArcAccessibilityHelperBridgeFactory() =
    default;

KeyedService* ArcAccessibilityHelperBridgeFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  auto* arc_service_manager = arc::ArcServiceManager::Get();
  if (!arc_service_manager ||
      arc_service_manager->browser_context() != context) {
    VLOG(1) << "Non ARC allowed browser context.";
    return nullptr;
  }

  return new ArcAccessibilityHelperBridge(
      Profile::FromBrowserContext(context),
      arc_service_manager->arc_bridge_service());
}

}  // namespace arc
