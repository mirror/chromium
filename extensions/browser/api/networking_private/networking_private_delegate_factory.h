// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef EXTENSIONS_BROWSER_API_NETWORKING_PRIVATE_NETWORKING_PRIVATE_DELEGATE_FACTORY_H_
#define EXTENSIONS_BROWSER_API_NETWORKING_PRIVATE_NETWORKING_PRIVATE_DELEGATE_FACTORY_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "build/build_config.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "extensions/browser/api/networking_private/networking_private_delegate.h"

namespace context {
class BrowserContext;
}

namespace extensions {

// Factory for creating NetworkingPrivateDelegate instances as a keyed service.
// NetworkingPrivateDelegate supports the networkingPrivate API.
class NetworkingPrivateDelegateFactory
    : public BrowserContextKeyedServiceFactory {
 public:
  static NetworkingPrivateDelegate* GetForBrowserContext(
      content::BrowserContext* browser_context);
  static NetworkingPrivateDelegateFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<NetworkingPrivateDelegateFactory>;

  NetworkingPrivateDelegateFactory();
  ~NetworkingPrivateDelegateFactory() override;

  // BrowserContextKeyedServiceFactory:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* browser_context) const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
  bool ServiceIsCreatedWithBrowserContext() const override;
  bool ServiceIsNULLWhileTesting() const override;

  DISALLOW_COPY_AND_ASSIGN(NetworkingPrivateDelegateFactory);
};

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_API_NETWORKING_PRIVATE_NETWORKING_PRIVATE_DELEGATE_FACTORY_H_
