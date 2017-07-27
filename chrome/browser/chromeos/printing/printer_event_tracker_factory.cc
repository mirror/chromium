// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/printing/printer_event_tracker_factory.h"

#include "base/lazy_instance.h"
#include "chrome/browser/chromeos/printing/printer_event_tracker.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace chromeos {
namespace {
base::LazyInstance<PrinterEventTrackerFactory>::DestructorAtExit
    g_printer_tracker = LAZY_INSTANCE_INITIALIZER;
}  // namespace

// static
PrinterEventTrackerFactory* PrinterEventTrackerFactory::GetInstance() {
  return g_printer_tracker.Pointer();
}

// static
PrinterEventTracker* PrinterEventTrackerFactory::GetForBrowserContext(
    content::BrowserContext* browser_context) {
  return static_cast<PrinterEventTracker*>(
      GetInstance()->GetServiceForBrowserContext(browser_context, true));
}

PrinterEventTrackerFactory::PrinterEventTrackerFactory()
    : BrowserContextKeyedServiceFactory(
          "PrinterEventTracker",
          BrowserContextDependencyManager::GetInstance()) {}
PrinterEventTrackerFactory::~PrinterEventTrackerFactory() = default;

// BrowserContextKeyedServiceFactory:
KeyedService* PrinterEventTrackerFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new PrinterEventTracker();
}

content::BrowserContext* PrinterEventTrackerFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextOwnInstanceInIncognito(context);
}

}  // namespace chromeos
