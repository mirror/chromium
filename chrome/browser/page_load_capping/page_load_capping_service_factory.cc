// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/page_load_capping/page_load_capping_service_factory.h"

#include "chrome/browser/page_load_capping/page_load_capping_service.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "content/public/browser/browser_context.h"

namespace {

base::LazyInstance<PageLoadCappingServiceFactory>::DestructorAtExit
    g_page_load_capping_factory = LAZY_INSTANCE_INITIALIZER;

}  // namespace

// static
PageLoadCappingService* PageLoadCappingServiceFactory::GetForBrowserContext(
    BrowserContext* browser_context) {
  return static_cast<PageLoadCappingService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
PageLoadCappingServiceFactory* PageLoadCappingServiceFactory::GetInstance() {
  return g_page_load_capping_factory.Pointer();
}

PageLoadCappingServiceFactory::PageLoadCappingServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "PageLoadCappingService",
          BrowserContextDependencyManager::GetInstance()) {}

PageLoadCappingServiceFactory::~PageLoadCappingServiceFactory() {}

KeyedService* PageLoadCappingServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new PageLoadCappingService();
}
