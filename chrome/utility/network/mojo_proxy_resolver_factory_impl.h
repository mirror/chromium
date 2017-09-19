// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_UTILITY_NETWORK_MOJO_PROXY_RESOLVER_FACTORY_IMPL_H_
#define CHROME_UTILITY_NETWORK_MOJO_PROXY_RESOLVER_FACTORY_IMPL_H_

#include <map>
#include <memory>
#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "net/interfaces/proxy_resolver_service.mojom.h"
#include "services/service_manager/public/cpp/service_context_ref.h"

namespace net {
class ProxyResolverV8TracingFactory;
}  // namespace net

namespace proxy_resolver {

class MojoProxyResolverFactoryImpl
    : public net::interfaces::ProxyResolverFactory {
 public:
  explicit MojoProxyResolverFactoryImpl(
      std::unique_ptr<service_manager::ServiceContextRef> service_ref);
  ~MojoProxyResolverFactoryImpl() override;

 protected:
  // Visible for tests.
  MojoProxyResolverFactoryImpl(
      std::unique_ptr<service_manager::ServiceContextRef> service_ref,
      std::unique_ptr<net::ProxyResolverV8TracingFactory>
          proxy_resolver_factory);

 private:
  class Job;

  // interfaces::ProxyResolverFactory override.
  void CreateResolver(
      const std::string& pac_script,
      net::interfaces::ProxyResolverRequest request,
      net::interfaces::ProxyResolverFactoryRequestClientPtr client) override;

  void RemoveJob(Job* job);

  const std::unique_ptr<service_manager::ServiceContextRef> service_ref_;

  const std::unique_ptr<net::ProxyResolverV8TracingFactory>
      proxy_resolver_impl_factory_;

  std::map<Job*, std::unique_ptr<Job>> jobs_;

  DISALLOW_COPY_AND_ASSIGN(MojoProxyResolverFactoryImpl);
};

}  // namespace proxy_resolver

#endif  // CHROME_UTILITY_NETWORK_MOJO_PROXY_RESOLVER_FACTORY_IMPL_H_
