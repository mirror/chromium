// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SIGNIN_IDENTITY_PROXY_FACTORY_H_
#define CHROME_BROWSER_SIGNIN_IDENTITY_PROXY_FACTORY_H_

#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace identity {
class IdentityProxy;
}

class Profile;

// Singleton that owns all IdentityProxy instances and associates them with
// Profiles. Listens for the Profile's destruction notification and cleans up
// the associated IdentityProxy.
class IdentityProxyFactory : public BrowserContextKeyedServiceFactory {
 public:
  static identity::IdentityProxy* GetForProfile(Profile* profile);

  // Returns an instance of the IdentityProxyFactory singleton.
  static IdentityProxyFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<IdentityProxyFactory>;

  IdentityProxyFactory();
  ~IdentityProxyFactory() override;

  // BrowserContextKeyedServiceFactory:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* profile) const override;
};

#endif  // CHROME_BROWSER_SIGNIN_IDENTITY_PROXY_FACTORY_H_
