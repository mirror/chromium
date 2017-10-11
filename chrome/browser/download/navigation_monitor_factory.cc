// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/download/navigation_monitor_factory.h"

#include "components/download/content/factory/download_service_factory.h"
#include "components/download/public/navigation_monitor.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

// static
NavigationMonitorFactory* NavigationMonitorFactory::GetInstance() {
  return base::Singleton<NavigationMonitorFactory>::get();
}

download::NavigationMonitor* NavigationMonitorFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<download::NavigationMonitor*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

NavigationMonitorFactory::NavigationMonitorFactory()
    : BrowserContextKeyedServiceFactory(
          "download::NavigationMonitor",
          BrowserContextDependencyManager::GetInstance()) {}

NavigationMonitorFactory::~NavigationMonitorFactory() = default;

KeyedService* NavigationMonitorFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return download::CreateNavigationMonitor();
}

content::BrowserContext* NavigationMonitorFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return context;
}
