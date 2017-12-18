// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/signin/identity_proxy_factory.h"

#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/signin/signin_manager_factory.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/signin/core/browser/signin_manager.h"
#include "services/identity/public/cpp/identity_proxy.h"

// Class that wraps IdentityProxy in a KeyedService (as IdentityProxy is a
// client-side library intended for use by any process, it would be a layering
// violation for IdentityProxy itself to have direct knowledge of KeyedService).
class IdentityProxyHolder : public KeyedService {
 public:
  explicit IdentityProxyHolder(Profile* profile)
      : identity_proxy_(SigninManagerFactory::GetForProfile(profile)) {}

  identity::IdentityProxy* identity_proxy() { return &identity_proxy_; }

 private:
  identity::IdentityProxy identity_proxy_;
};

IdentityProxyFactory::IdentityProxyFactory()
    : BrowserContextKeyedServiceFactory(
          "IdentityProxy",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(SigninManagerFactory::GetInstance());
}

IdentityProxyFactory::~IdentityProxyFactory() {}

// static
identity::IdentityProxy* IdentityProxyFactory::GetForProfile(Profile* profile) {
  IdentityProxyHolder* holder = static_cast<IdentityProxyHolder*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));

  return holder->identity_proxy();
}

// static
IdentityProxyFactory* IdentityProxyFactory::GetInstance() {
  return base::Singleton<IdentityProxyFactory>::get();
}

KeyedService* IdentityProxyFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new IdentityProxyHolder(Profile::FromBrowserContext(context));
}
