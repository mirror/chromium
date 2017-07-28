// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/profiling/profiling_service.h"

#include "chrome/profiling/memlog_impl.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace profiling {

namespace {
// Simple handler to actually create a MemlogInfo when the ServiceManager
// connection pipe receives a request for the Memlog service.
//
// The service manifest declares that only 1 Memlog service exists for all
// "users" meaning this should only ever get called once per broker cluster.
void OnMemlogRequest(service_manager::ServiceContextRefFactory* ref_factory,
                     mojom::MemlogRequest request) {
  mojo::MakeStrongBinding(
      base::MakeUnique<MemlogImpl>(ref_factory->CreateRef()),
      std::move(request));
}
}  // namespace

ProfilingService::ProfilingService() : weak_factory_(this) {}

ProfilingService::~ProfilingService() {}

std::unique_ptr<service_manager::Service> ProfilingService::CreateService() {
  return base::MakeUnique<ProfilingService>();
}

void ProfilingService::OnStart() {
  ref_factory_.reset(new service_manager::ServiceContextRefFactory(base::Bind(
      &ProfilingService::MaybeRequestQuitDelayed, base::Unretained(this))));
  registry_.AddInterface(base::Bind(&OnMemlogRequest, ref_factory_.get()));
}

void ProfilingService::OnBindInterface(
    const service_manager::BindSourceInfo& source_info,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  registry_.BindInterface(interface_name, std::move(interface_pipe));

  // TODO(ajwong): Maybe signal shutdown when all interfaces are closed?  What
  // does ServiceManager actually do?
}

void ProfilingService::MaybeRequestQuitDelayed() {
  // TODO(ajwong): What does this and the MaybeRequestQuit() function actually
  // do? This is just cargo-culted from another mojo service.
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE,
      base::Bind(&ProfilingService::MaybeRequestQuit,
                 weak_factory_.GetWeakPtr()),
      base::TimeDelta::FromSeconds(5));
}

void ProfilingService::MaybeRequestQuit() {
  DCHECK(ref_factory_);
  if (ref_factory_->HasNoRefs())
    context()->RequestQuit();
}

}  // namespace profiling
