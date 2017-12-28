// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_HISTORY_NAV_TREE_FACTORY_H_
#define CHROME_BROWSER_HISTORY_NAV_TREE_FACTORY_H_

#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "components/keyed_service/core/service_access_type.h"

class NavTree;
class Profile;

class NavTreeFactory : public BrowserContextKeyedServiceFactory {
 public:
  static NavTreeFactory* GetInstance();
  static NavTree* GetForProfile(Profile* profile, ServiceAccessType sat);

 private:
  friend struct base::DefaultSingletonTraits<NavTreeFactory>;

  NavTreeFactory();
  ~NavTreeFactory() override;

  // BrowserContextKeyedServiceFactory:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
  bool ServiceIsNULLWhileTesting() const override;
};

#endif  // CHROME_BROWSER_HISTORY_NAV_TREE_FACTORY_H_
