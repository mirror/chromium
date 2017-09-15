// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/test/test_mojo_proxy_resolver_factory.h"

#include "base/memory/ptr_util.h"
#include "content/public/network/mojo_proxy_resolver_factory_impl.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace content {

TestMojoProxyResolverFactory* TestMojoProxyResolverFactory::GetInstance() {
  return base::Singleton<TestMojoProxyResolverFactory>::get();
}

std::unique_ptr<base::ScopedClosureRunner>
TestMojoProxyResolverFactory::CreateResolver(
    const std::string& pac_script,
    mojo::InterfaceRequest<mojom::ProxyResolver> req,
    mojom::ProxyResolverFactoryRequestClientPtr client) {
  resolver_created_ = true;
  factory_->CreateResolver(pac_script, std::move(req), std::move(client));
  return nullptr;
}

TestMojoProxyResolverFactory::TestMojoProxyResolverFactory() {
  mojo::MakeStrongBinding(std::make_unique<MojoProxyResolverFactoryImpl>(),
                          mojo::MakeRequest(&factory_));
}

TestMojoProxyResolverFactory::~TestMojoProxyResolverFactory() = default;

}  // namespace content
