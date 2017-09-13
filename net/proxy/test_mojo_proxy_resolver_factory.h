// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_PROXY_TEST_MOJO_PROXY_RESOLVER_FACTORY_H_
#define NET_PROXY_TEST_MOJO_PROXY_RESOLVER_FACTORY_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "net/interfaces/proxy_resolver_service.mojom.h"

namespace net {

// MojoProxyResolverFactory that runs PAC scripts in-process, for tests.
class TestMojoProxyResolverFactory : public interfaces::ProxyResolverFactory {
 public:
  TestMojoProxyResolverFactory();
  ~TestMojoProxyResolverFactory() override;

  // Returns true if CreateResolver was called.
  bool resolver_created() const { return resolver_created_; }

  interfaces::ProxyResolverFactoryPtr CreateFactoryInterface();

 private:
  // Overridden from interfaces::ProxyResolverFactory:
  void CreateResolver(
      const std::string& pac_script,
      interfaces::ProxyResolverRequest req,
      interfaces::ProxyResolverFactoryRequestClientPtr client) override;
  void OnResolverDestroyed() override;

  interfaces::ProxyResolverFactoryPtr factory_;

  mojo::Binding<ProxyResolverFactory> binding_;

  bool resolver_created_ = false;

  DISALLOW_COPY_AND_ASSIGN(TestMojoProxyResolverFactory);
};

}  // namespace net

#endif  // NET_PROXY_TEST_MOJO_PROXY_RESOLVER_FACTORY_H_
