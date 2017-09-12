// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_NET_CHROME_MOJO_PROXY_RESOLVER_FACTORY_H_
#define CHROME_BROWSER_NET_CHROME_MOJO_PROXY_RESOLVER_FACTORY_H_

#include <stddef.h>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/thread_checker.h"
#include "base/timer/timer.h"
#include "net/interfaces/proxy_resolver_service.mojom.h"

#if !defined(OS_ANDROID)
namespace content {
class UtilityProcessHost;
}
#endif

namespace base {
template <typename Type>
struct DefaultSingletonTraits;
}  // namespace base

// A factory used to create connections to Mojo proxy resolver services.  On
// Android, the proxy resolvers will run in the browser process, and on other
// platforms, they'll all be run in the same utility process. Utility process
// crashes are detected and the utility process is automatically restarted.
class ChromeMojoProxyResolverFactory {
 public:
  static ChromeMojoProxyResolverFactory* GetInstance();

  // Creates a new net::interfaces::ProxyResolverFactoryPtr, which proxies
  // ProxyResolver creation through the ChromeMojoProxyResolverFactory.
  net::interfaces::ProxyResolverFactoryPtr CreateFactoryInterface();

  net::interfaces::ProxyResolverFactory* get_utility_factory_for_testing()
      const {
    return utility_process_resolver_factory_.get();
  }

  // Sets the idle timeout for shutting down the utility process when there are
  // no live ProxyResolvers.
  static void SetUtilityProcessIdleTimeoutSecsForTesting(int seconds);

 private:
  friend struct base::DefaultSingletonTraits<ChromeMojoProxyResolverFactory>;

  // Each ResolverFactoryChannel manages a single mojo connection, tracking the
  // number of live resolvers it has issued.
  class ResolverFactoryChannel;

  ChromeMojoProxyResolverFactory();
  ~ChromeMojoProxyResolverFactory();

  // Creates the proxy resolver factory. On desktop, creates a new utility
  // process before creating it out of process. On Android, creates it on the
  // current thread.
  net::interfaces::ProxyResolverFactory* ResolverFactory();

  // Destroys |resolver_factory_|.
  void DestroyFactory();

  // Invoked once an idle timeout has elapsed after all proxy resolvers are
  // destroyed.
  void OnIdleTimeout();

  // Called when a ResolverFactoryChannel's connection is closed. Destroys
  // |binding|.
  void OnConnectionClosed(ResolverFactoryChannel* binding);

  // Called whenever the number of proxy resolvers changes.
  void OnNumProxyResolversChanged(int delta);

  // The real ProxyResolverFactory that, on desktop platforms, lives in the
  // utility process.
  net::interfaces::ProxyResolverFactoryPtr utility_process_resolver_factory_;

#if !defined(OS_ANDROID)
  base::WeakPtr<content::UtilityProcessHost> weak_utility_process_host_;
#endif
  std::set<std::unique_ptr<ResolverFactoryChannel>> channels_;

  int num_proxy_resolvers_ = 0;

  base::OneShotTimer idle_timer_;

  base::ThreadChecker thread_checker_;

  DISALLOW_COPY_AND_ASSIGN(ChromeMojoProxyResolverFactory);
};

#endif  // CHROME_BROWSER_NET_CHROME_MOJO_PROXY_RESOLVER_FACTORY_H_
