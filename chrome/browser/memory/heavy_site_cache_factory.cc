// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/memory/heavy_site_cache_factory.h"

#include "base/memory/singleton.h"
#include "chrome/browser/memory/heavy_site_cache.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/keyed_service/core/keyed_service.h"

namespace memory {

// static
HeavySiteCache* HeavySiteCacheFactory::GetForProfile(Profile* profile) {
  return static_cast<HeavySiteCache*>(
      GetInstance()->GetServiceForBrowserContext(profile, true /* create */));
}

// static
HeavySiteCacheFactory* HeavySiteCacheFactory::GetInstance() {
  return base::Singleton<HeavySiteCacheFactory>::get();
}

HeavySiteCacheFactory::HeavySiteCacheFactory()
    : BrowserContextKeyedServiceFactory(
          "HeavySiteCacheFactory",
          BrowserContextDependencyManager::GetInstance()) {}
HeavySiteCacheFactory::~HeavySiteCacheFactory() = default;

KeyedService* HeavySiteCacheFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  // TODO(csharrison): Tune with variation params.
  return new HeavySiteCache(static_cast<Profile*>(context),
                            10u /* max_cache_size */);
}

content::BrowserContext* HeavySiteCacheFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextOwnInstanceInIncognito(context);
}

}  // namespace memory
