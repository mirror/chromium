// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_NETWORK_MOJO_PROXY_RESOLVER_FACTORY_IMPL_H_
#define CONTENT_PUBLIC_NETWORK_MOJO_PROXY_RESOLVER_FACTORY_IMPL_H_

#include <map>

#include "base/callback.h"
#include "base/macros.h"
#include "content/common/content_export.h"
#include "content/public/common/proxy_resolver.mojom.h"

namespace net {
class ProxyResolverV8TracingFactory;
}  // namespace net

namespace content {

class CONTENT_EXPORT MojoProxyResolverFactoryImpl
    : public mojom::ProxyResolverFactory {
 public:
  MojoProxyResolverFactoryImpl();
  explicit MojoProxyResolverFactoryImpl(
      std::unique_ptr<net::ProxyResolverV8TracingFactory>
          proxy_resolver_factory);

  ~MojoProxyResolverFactoryImpl() override;

  // mojom::ProxyResolverFactory override.
  void CreateResolver(
      const std::string& pac_script,
      mojom::ProxyResolverRequest request,
      mojom::ProxyResolverFactoryRequestClientPtr client) override;

 private:
  class Job;

  void RemoveJob(Job* job);

  const std::unique_ptr<net::ProxyResolverV8TracingFactory>
      proxy_resolver_impl_factory_;

  std::map<Job*, std::unique_ptr<Job>> jobs_;

  DISALLOW_COPY_AND_ASSIGN(MojoProxyResolverFactoryImpl);
};

}  // namespace content

#endif  // CONTENT_PUBLIC_NETWORK_MOJO_PROXY_RESOLVER_FACTORY_IMPL_H_
