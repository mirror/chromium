// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/networking_onc/networking_onc_event_router_factory.h"

#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "content/public/browser/browser_context.h"
#include "extensions/browser/api/networking_onc/networking_onc_delegate_factory.h"
#include "extensions/browser/api/networking_onc/networking_onc_event_router.h"
#include "extensions/browser/extension_system_provider.h"
#include "extensions/browser/extensions_browser_client.h"

namespace extensions {

// static
NetworkingOncEventRouter* NetworkingOncEventRouterFactory::GetForProfile(
    content::BrowserContext* context) {
  return static_cast<NetworkingOncEventRouter*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

// static
NetworkingOncEventRouterFactory*
NetworkingOncEventRouterFactory::GetInstance() {
  return base::Singleton<NetworkingOncEventRouterFactory>::get();
}

NetworkingOncEventRouterFactory::NetworkingOncEventRouterFactory()
    : BrowserContextKeyedServiceFactory(
          "NetworkingOncEventRouter",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(ExtensionsBrowserClient::Get()->GetExtensionSystemFactory());
  DependsOn(NetworkingOncDelegateFactory::GetInstance());
}

NetworkingOncEventRouterFactory::~NetworkingOncEventRouterFactory() {}

KeyedService* NetworkingOncEventRouterFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return NetworkingOncEventRouter::Create(context);
}

content::BrowserContext*
NetworkingOncEventRouterFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return ExtensionsBrowserClient::Get()->GetOriginalContext(context);
}

bool NetworkingOncEventRouterFactory::ServiceIsCreatedWithBrowserContext()
    const {
  return true;
}

bool NetworkingOncEventRouterFactory::ServiceIsNULLWhileTesting() const {
  return true;
}

}  // namespace extensions
