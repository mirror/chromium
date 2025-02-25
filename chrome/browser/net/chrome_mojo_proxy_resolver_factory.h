// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_NET_CHROME_MOJO_PROXY_RESOLVER_FACTORY_H_
#define CHROME_BROWSER_NET_CHROME_MOJO_PROXY_RESOLVER_FACTORY_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "services/proxy_resolver/public/interfaces/proxy_resolver.mojom.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/service_manager/public/mojom/connector.mojom.h"

// ProxyResolverFactory that acts as a proxy to the proxy resolver service.
// Starts the service as needed, and maintains no active mojo pipes to it,
// so that it's automatically shut down as needed.
//
// ChromeMojoProxyResolverFactories must be created and used only on the UI
// thread.
class ChromeMojoProxyResolverFactory
    : public proxy_resolver::mojom::ProxyResolverFactory {
 public:
  ChromeMojoProxyResolverFactory();
  ~ChromeMojoProxyResolverFactory() override;

  // Convenience method that creates a ProxyResolverFactory, and Mojo strong
  // binding wrapping it.
  static proxy_resolver::mojom::ProxyResolverFactoryPtr
  CreateWithStrongBinding();

  // proxy_resolver::mojom::ProxyResolverFactory implementation:
  void CreateResolver(
      const std::string& pac_script,
      proxy_resolver::mojom::ProxyResolverRequest req,
      proxy_resolver::mojom::ProxyResolverFactoryRequestClientPtr client)
      override;

 private:
  std::unique_ptr<service_manager::Connector> service_manager_connector_;

  DISALLOW_COPY_AND_ASSIGN(ChromeMojoProxyResolverFactory);
};

#endif  // CHROME_BROWSER_NET_CHROME_MOJO_PROXY_RESOLVER_FACTORY_H_
