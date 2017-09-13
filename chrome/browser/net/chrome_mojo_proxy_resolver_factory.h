// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_NET_CHROME_MOJO_PROXY_RESOLVER_FACTORY_H_
#define CHROME_BROWSER_NET_CHROME_MOJO_PROXY_RESOLVER_FACTORY_H_

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/thread_checker.h"
#include "mojo/public/cpp/bindings/binding_set.h"
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

// A ProxyResolverFactory that acts as a proxy for another ProxyResolverFactory
// that does the actual work.  On Android, the proxy resolvers will run in the
// browser process, and on other platforms, they'll all be run in the same
// utility process. The utility process is started as needed. The main purpose
// of this class is so that the utility process will only be run when needed.
class ChromeMojoProxyResolverFactory
    : public net::interfaces::ProxyResolverFactory {
 public:
  static ChromeMojoProxyResolverFactory* GetInstance();

  // Binds a factory request to |this|.
  void BindRequest(
      net::interfaces::ProxyResolverFactoryRequest factory_request);

 private:
  friend struct base::DefaultSingletonTraits<ChromeMojoProxyResolverFactory>;

  ChromeMojoProxyResolverFactory();
  ~ChromeMojoProxyResolverFactory();

  // Creates the proxy resolver factory. On desktop, creates a new utility
  // process before creating it out of process. On Android, creates it on the
  // current thread.
  void CreateFactory();

  // net::interfaces::ProxyResolverFactory implementation:
  void CreateResolver(
      const std::string& pac_script,
      mojo::InterfaceRequest<net::interfaces::ProxyResolver> req,
      net::interfaces::ProxyResolverFactoryRequestClientPtr client) override;

  mojo::BindingSet<net::interfaces::ProxyResolverFactory> binding_set_;

  net::interfaces::ProxyResolverFactoryPtr resolver_factory_;

  // The real ProxyResolverFactory that, on desktop platforms, lives in the
  // utility process.
  net::interfaces::ProxyResolverFactoryPtr utility_process_resolver_factory_;

#if !defined(OS_ANDROID)
  base::WeakPtr<content::UtilityProcessHost> weak_utility_process_host_;
#endif

  base::ThreadChecker thread_checker_;

  DISALLOW_COPY_AND_ASSIGN(ChromeMojoProxyResolverFactory);
};

#endif  // CHROME_BROWSER_NET_CHROME_MOJO_PROXY_RESOLVER_FACTORY_H_
