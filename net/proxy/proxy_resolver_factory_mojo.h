// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_PROXY_PROXY_RESOLVER_FACTORY_MOJO_H_
#define NET_PROXY_PROXY_RESOLVER_FACTORY_MOJO_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "net/base/completion_callback.h"
#include "net/interfaces/proxy_resolver_service.mojom.h"
#include "net/proxy/proxy_resolver_factory.h"

namespace net {
class HostResolver;
class NetLog;
class ProxyResolverErrorObserver;
class ProxyResolverScriptData;

// Implementation of ProxyResolverFactory that connects to a Mojo service to
// create implementations of a Mojo proxy resolver to back a ProxyResolverMojo.
class ProxyResolverFactoryMojo : public ProxyResolverFactory {
 public:
  ProxyResolverFactoryMojo(
      interfaces::ProxyResolverFactoryPtr mojo_proxy_factory,
      HostResolver* host_resolver,
      const base::Callback<std::unique_ptr<ProxyResolverErrorObserver>()>&
          error_observer_factory,
      NetLog* net_log);
  ~ProxyResolverFactoryMojo() override;

  // ProxyResolverFactory override.
  int CreateProxyResolver(
      const scoped_refptr<ProxyResolverScriptData>& pac_script,
      std::unique_ptr<ProxyResolver>* resolver,
      const CompletionCallback& callback,
      std::unique_ptr<Request>* request) override;

 private:
  class Job;

  // Called when a created ProxyResolver is informed, to notify
  // |mojo_proxy_factory_| of the resolver's destruction.
  void OnResolverDestroyed();

  interfaces::ProxyResolverFactoryPtr mojo_proxy_factory_;
  HostResolver* const host_resolver_;
  const base::Callback<std::unique_ptr<ProxyResolverErrorObserver>()>
      error_observer_factory_;
  NetLog* const net_log_;

  base::WeakPtrFactory<ProxyResolverFactoryMojo> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ProxyResolverFactoryMojo);
};

}  // namespace net

#endif  // NET_PROXY_PROXY_RESOLVER_FACTORY_MOJO_H_
