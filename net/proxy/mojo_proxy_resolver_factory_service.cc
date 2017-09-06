// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/proxy/mojo_proxy_resolver_factory_service.h"

#include "build/build_config.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "net/proxy/mojo_proxy_resolver_factory_impl.h"

namespace net {

namespace {

void OnProxyResolverFactoryRequest(
    service_manager::ServiceContextRefFactory* ref_factory,
    net::interfaces::ProxyResolverFactoryRequest request) {
  mojo::MakeStrongBinding(
      base::MakeUnique<MojoProxyResolverFactoryImpl>(ref_factory->CreateRef()),
      std::move(request));
}

}  // namespace

MojoProxyResolverFactoryService::MojoProxyResolverFactoryService() {}

MojoProxyResolverFactoryService::~MojoProxyResolverFactoryService() {}

std::unique_ptr<service_manager::Service>
MojoProxyResolverFactoryService::CreateService() {
  return base::MakeUnique<MojoProxyResolverFactoryService>();
}

void MojoProxyResolverFactoryService::OnStart() {
  ref_factory_.reset(new service_manager::ServiceContextRefFactory(
      base::Bind(&service_manager::ServiceContext::RequestQuit,
                 base::Unretained(context()))));
  registry_.AddInterface(
      base::Bind(&OnProxyResolverFactoryRequest, ref_factory_.get()));
}

void MojoProxyResolverFactoryService::OnBindInterface(
    const service_manager::BindSourceInfo& source_info,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  registry_.BindInterface(interface_name, std::move(interface_pipe));
}

}  // namespace net