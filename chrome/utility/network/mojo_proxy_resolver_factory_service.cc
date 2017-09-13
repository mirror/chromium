// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/utility/network/mojo_proxy_resolver_factory_service.h"

#include <utility>

#include "build/build_config.h"
#include "chrome/utility/network/mojo_proxy_resolver_factory_impl.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace proxy_resolver {

namespace {

void OnProxyResolverFactoryRequest(
    service_manager::ServiceContextRefFactory* ref_factory,
    net::interfaces::ProxyResolverFactoryRequest request) {
  mojo::MakeStrongBinding(
      base::MakeUnique<MojoProxyResolverFactoryImpl>(ref_factory->CreateRef()),
      std::move(request));
}

}  // namespace

MojoProxyResolverFactoryService::MojoProxyResolverFactoryService()
    : weak_factory_(this) {}

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

void MojoProxyResolverFactoryService::MaybeRequestQuitDelayed() {
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE,
      base::Bind(&MojoProxyResolverFactoryService::MaybeRequestQuit,
                 weak_factory_.GetWeakPtr()),
      base::TimeDelta::FromSeconds(5));
}

void MojoProxyResolverFactoryService::MaybeRequestQuit() {
  DCHECK(ref_factory_);
  if (ref_factory_->HasNoRefs())
    context()->RequestQuit();
}

}  // namespace proxy_resolver
