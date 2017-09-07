// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/networking_onc/networking_onc_delegate_factory.h"

#include <utility>
#include <vector>

#include "build/build_config.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "content/public/browser/browser_thread.h"
#include "extensions/browser/extensions_browser_client.h"

#if defined(OS_CHROMEOS)
#include "extensions/browser/api/networking_onc/networking_onc_chromeos.h"
#elif defined(OS_LINUX)
#include "extensions/browser/api/networking_onc/networking_onc_linux.h"
#elif defined(OS_WIN) || defined(OS_MACOSX)
#include "components/wifi/wifi_service.h"
#include "extensions/browser/api/networking_onc/networking_onc_service_client.h"
#endif

namespace extensions {

using content::BrowserContext;

// static
NetworkingOncDelegate* NetworkingOncDelegateFactory::GetForBrowserContext(
    BrowserContext* browser_context) {
  return static_cast<NetworkingOncDelegate*>(
      GetInstance()->GetServiceForBrowserContext(browser_context, true));
}

// static
NetworkingOncDelegateFactory* NetworkingOncDelegateFactory::GetInstance() {
  return base::Singleton<NetworkingOncDelegateFactory>::get();
}

NetworkingOncDelegateFactory::NetworkingOncDelegateFactory()
    : BrowserContextKeyedServiceFactory(
          "NetworkingOncDelegate",
          BrowserContextDependencyManager::GetInstance()) {}

NetworkingOncDelegateFactory::~NetworkingOncDelegateFactory() {}

KeyedService* NetworkingOncDelegateFactory::BuildServiceInstanceFor(
    BrowserContext* browser_context) const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  NetworkingOncDelegate* delegate;
#if defined(OS_CHROMEOS)
  delegate = new NetworkingOncChromeOS(browser_context);
#elif defined(OS_LINUX)
  delegate = new NetworkingOncLinux();
#elif defined(OS_WIN) || defined(OS_MACOSX)
  std::unique_ptr<wifi::WiFiService> wifi_service(wifi::WiFiService::Create());
  delegate = new NetworkingOncServiceClient(std::move(wifi_service));
#else
  NOTREACHED();
  delegate = nullptr;
#endif

  return delegate;
}

BrowserContext* NetworkingOncDelegateFactory::GetBrowserContextToUse(
    BrowserContext* context) const {
  return ExtensionsBrowserClient::Get()->GetOriginalContext(context);
}

bool NetworkingOncDelegateFactory::ServiceIsCreatedWithBrowserContext() const {
  return false;
}

bool NetworkingOncDelegateFactory::ServiceIsNULLWhileTesting() const {
  return false;
}

}  // namespace extensions
