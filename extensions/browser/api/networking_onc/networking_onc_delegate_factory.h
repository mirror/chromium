// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef EXTENSIONS_BROWSER_API_NETWORKING_ONC_NETWORKING_ONC_DELEGATE_FACTORY_H_
#define EXTENSIONS_BROWSER_API_NETWORKING_ONC_NETWORKING_ONC_DELEGATE_FACTORY_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "build/build_config.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "extensions/browser/api/networking_onc/networking_onc_delegate.h"

namespace context {
class BrowserContext;
}

namespace extensions {

// Factory for creating NetworkingOncDelegate instances as a keyed service.
// NetworkingOncDelegate supports the networkingOnc API.
class NetworkingOncDelegateFactory : public BrowserContextKeyedServiceFactory {
 public:
  static NetworkingOncDelegate* GetForBrowserContext(
      content::BrowserContext* browser_context);
  static NetworkingOncDelegateFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<NetworkingOncDelegateFactory>;

  NetworkingOncDelegateFactory();
  ~NetworkingOncDelegateFactory() override;

  // BrowserContextKeyedServiceFactory:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* browser_context) const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
  bool ServiceIsCreatedWithBrowserContext() const override;
  bool ServiceIsNULLWhileTesting() const override;

  DISALLOW_COPY_AND_ASSIGN(NetworkingOncDelegateFactory);
};

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_API_NETWORKING_ONC_NETWORKING_ONC_DELEGATE_FACTORY_H_
