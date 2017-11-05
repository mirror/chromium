// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_NET_PROXY_RESOLVER_MANAGER_H_
#define CHROME_BROWSER_NET_PROXY_RESOLVER_MANAGER_H_

#include <stddef.h>

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/threading/thread_checker.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/proxy_resolver/public/interfaces/proxy_resolver.mojom.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/service_manager/public/interfaces/connector.mojom.h"

// Class to manage access to the proxy resolver service by creating
// ProxyResolverFactories that track the number of live ProxyResolvers, while
// using the proxy resolver service to create ProxyResolvers (Which are out of
// process on desktop, but in-process on Android). Starts and shuts down the
// proxy resolver service as needed, to minimize resource usage.
class ProxyResolverManager
    : public proxy_resolver::mojom::ProxyResolverFactory {
 public:
  ProxyResolverManager();
  ~ProxyResolverManager() override;

  static proxy_resolver::mojom::ProxyResolverFactoryPtr
  CreateWithStrongBinding();

  // proxy_resolver::mojom::ProxyResolverFactory implementation:
  void CreateResolver(
      const std::string& pac_script,
      proxy_resolver::mojom::ProxyResolverRequest req,
      proxy_resolver::mojom::ProxyResolverFactoryRequestClientPtr client)
      override;

  // Initializes the ServiceManager's connector if it hasn't been already.
  void InitServiceManagerConnector();

  std::unique_ptr<service_manager::Connector> service_manager_connector_;

  base::ThreadChecker thread_checker_;

  DISALLOW_COPY_AND_ASSIGN(ProxyResolverManager);
};

#endif  // CHROME_BROWSER_NET_PROXY_RESOLVER_MANAGER_H_
