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
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/proxy_resolver/public/interfaces/proxy_resolver.mojom.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/service_manager/public/interfaces/connector.mojom.h"

namespace base {
template <typename Type>
struct DefaultSingletonTraits;
}  // namespace base

// Class to manage access to the proxy resolver service by creating
// ProxyResolverFactories that track the number of live ProxyResolvers, while
// using the proxy resolver service to create ProxyResolvers (Which are out of
// process on desktop, but in-process on Android). Starts and shuts down the
// proxy resolver service as needed, to minimize resource usage.
class ProxyResolverManager {
 public:
  static ProxyResolverManager* GetInstance();

  // Creates a new net::interfaces::ProxyResolverFactoryPtr, which proxies
  // ProxyResolver creation through |this|.
  proxy_resolver::mojom::ProxyResolverFactoryPtr CreateFactoryInterface();

  // Used by tests to override the default timeout used when no resolver is
  // connected before disconnecting the factory (and causing the service to
  // stop).
  void SetFactoryIdleTimeoutForTests(const base::TimeDelta& timeout);

 private:
  friend struct base::DefaultSingletonTraits<ProxyResolverManager>;

  // Each ResolverFactoryChannel manages a single mojo connection, tracking the
  // number of live resolvers it has issued.
  class ResolverFactoryChannel;

  ProxyResolverManager();
  ~ProxyResolverManager();

  // Initializes the ServiceManager's connector if it hasn't been already.
  void InitServiceManagerConnector();

  // Returns the the proxy resolver factory, creating it if needed. On desktop,
  // creates a new utility process before creating it out of process. On
  // Android, creates it on the current thread.
  proxy_resolver::mojom::ProxyResolverFactory* ResolverFactory();

  // Destroys |resolver_factory_|.
  void DestroyFactory();

  // Invoked once an idle timeout has elapsed after all proxy resolvers are
  // destroyed.
  void OnIdleTimeout();

  // Called when a ResolverFactoryChannel's connection is closed. Destroys
  // |binding|.
  void OnConnectionClosed(ResolverFactoryChannel* binding);

  // Should be called when the number of ProxyResolvers changes. If no
  // ProxyResolvers exist, starts a timeout to shut down the utility process.
  // Otherwise, stops the shutdown timer if its running.
  void OnNumProxyResolversChanged();

  // The real ProxyResolverFactory that, on desktop platforms, lives in a
  // utility process.
  proxy_resolver::mojom::ProxyResolverFactoryPtr resolver_factory_;

  std::unique_ptr<service_manager::Connector> service_manager_connector_;

  std::set<std::unique_ptr<ResolverFactoryChannel>> channels_;

  base::OneShotTimer idle_timer_;

  base::ThreadChecker thread_checker_;

  base::TimeDelta factory_idle_timeout_;

  DISALLOW_COPY_AND_ASSIGN(ProxyResolverManager);
};

#endif  // CHROME_BROWSER_NET_PROXY_RESOLVER_MANAGER_H_
