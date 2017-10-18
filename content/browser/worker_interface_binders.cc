// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/worker_interface_binders.h"

#include "services/service_manager/public/cpp/binder_registry.h"

namespace content {
namespace {

using WorkerBinderRegistry =
    service_manager::BinderRegistryWithArgs<RenderProcessHost*,
                                            const url::Origin&>;

WorkerBinderRegistry& GetWorkerBinderRegistry() {
  static bool initialized = false;
  CR_DEFINE_STATIC_LOCAL(WorkerBinderRegistry, registry, ());
  if (!initialized) {
    initialized = true;
  }
  return registry;
}

}  // namespace

void BindWorkerInterface(const std::string& interface_name,
                         mojo::ScopedMessagePipeHandle interface_pipe,
                         RenderProcessHost* host,
                         const url::Origin& origin) {
  return GetWorkerBinderRegistry().BindInterface(
      interface_name, std::move(interface_pipe), host, origin);
}

}  // namespace content
