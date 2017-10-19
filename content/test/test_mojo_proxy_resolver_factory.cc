// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/test/test_mojo_proxy_resolver_factory.h"

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/proxy_resolver/proxy_resolver_factory_impl.h"

namespace content {

TestMojoProxyResolverFactory::TestMojoProxyResolverFactory() : binding_(this) {
  mojo::MakeStrongBinding(
      std::make_unique<proxy_resolver::ProxyResolverFactoryImpl>(),
      mojo::MakeRequest(&factory_));
}

TestMojoProxyResolverFactory::~TestMojoProxyResolverFactory() = default;

void TestMojoProxyResolverFactory::CreateResolver(
    const std::string& pac_script,
    proxy_resolver::mojom::ProxyResolverRequest req,
    proxy_resolver::mojom::ProxyResolverFactoryRequestClientPtr client) {
  resolver_created_ = true;
  factory_->CreateResolver(pac_script, std::move(req), std::move(client));
}

void TestMojoProxyResolverFactory::OnResolverDestroyed() {}

proxy_resolver::mojom::ProxyResolverFactoryPtr
TestMojoProxyResolverFactory::CreateFactoryInterface() {
  DCHECK(!binding_.is_bound());
  proxy_resolver::mojom::ProxyResolverFactoryPtr mojo_factory;
  binding_.Bind(mojo::MakeRequest(&mojo_factory));
  return mojo_factory;
}

}  // namespace content
