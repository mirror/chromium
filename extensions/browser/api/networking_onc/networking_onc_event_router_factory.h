// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_API_NETWORKING_ONC_NETWORKING_ONC_EVENT_ROUTER_FACTORY_H_
#define EXTENSIONS_BROWSER_API_NETWORKING_ONC_NETWORKING_ONC_EVENT_ROUTER_FACTORY_H_

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace extensions {

class NetworkingOncEventRouter;

// This is a factory class used by the BrowserContextDependencyManager
// to instantiate the networking event router per profile (since the extension
// event router is per profile).
class NetworkingOncEventRouterFactory
    : public BrowserContextKeyedServiceFactory {
 public:
  // Returns the NetworkingOncEventRouter for |profile|, creating it if
  // it is not yet created.
  static NetworkingOncEventRouter* GetForProfile(
      content::BrowserContext* context);

  // Returns the NetworkingOncEventRouterFactory instance.
  static NetworkingOncEventRouterFactory* GetInstance();

 protected:
  // BrowserContextKeyedBaseFactory overrides:
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
  bool ServiceIsCreatedWithBrowserContext() const override;
  bool ServiceIsNULLWhileTesting() const override;

 private:
  friend struct base::DefaultSingletonTraits<NetworkingOncEventRouterFactory>;

  NetworkingOncEventRouterFactory();
  ~NetworkingOncEventRouterFactory() override;

  // BrowserContextKeyedServiceFactory:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* profile) const override;

  DISALLOW_COPY_AND_ASSIGN(NetworkingOncEventRouterFactory);
};

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_API_NETWORKING_ONC_NETWORKING_ONC_EVENT_ROUTER_FACTORY_H_
