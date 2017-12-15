// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/history/nav_tree_factory.h"

#include "chrome/browser/history/nav_tree.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

NavTreeFactory::NavTreeFactory()
    : BrowserContextKeyedServiceFactory(
          "NavTree",
          BrowserContextDependencyManager::GetInstance()) {}

NavTreeFactory::~NavTreeFactory() {}

KeyedService* NavTreeFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  Profile* profile = static_cast<Profile*>(context);
  std::unique_ptr<NavTree> nav_tree = std::make_unique<NavTree>(profile);
  return nav_tree.release();
}

content::BrowserContext* NavTreeFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

bool NavTreeFactory::ServiceIsNULLWhileTesting() const {
  return true;
}

// static
NavTreeFactory* NavTreeFactory::GetInstance() {
  return base::Singleton<NavTreeFactory>::get();
}

// static
NavTree* NavTreeFactory::GetForProfile(Profile* profile,
                                       ServiceAccessType sat) {
  if (sat != ServiceAccessType::EXPLICIT_ACCESS)
    return nullptr;
  return static_cast<NavTree*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}
