// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/worker_interface_binders.h"

#include <utility>

#include "base/bind.h"
#include "content/browser/permissions/permission_service_context.h"
#include "content/browser/renderer_host/render_process_host_impl.h"
#include "content/public/browser/browser_thread.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "url/origin.h"

namespace content {
namespace {

// A container of parametized and unparameterized BinderRegistries for
// content-layer interfaces exposed to web workers.
class WorkerInterfaceBinders {
 public:
  WorkerInterfaceBinders() {
    InitializeParameterizedBinderRegistry();
    // TODO(sammc): Populate |binder_registry_|.
  }

  // Bind an interface request |interface_pipe| for |interface_name| received
  // from a web worker with origin |origin| hosted in the renderer |host|.
  void BindInterface(const std::string& interface_name,
                     mojo::ScopedMessagePipeHandle interface_pipe,
                     RenderProcessHost* host,
                     const url::Origin& origin) {
    if (parameterized_binder_registry_.TryBindInterface(
            interface_name, &interface_pipe, host, origin)) {
      return;
    }

    if (binder_registry_.TryBindInterface(interface_name, &interface_pipe))
      return;

    // TODO(sammc): Forward unhandled requests to the embedder.
  }

 private:
  void InitializeParameterizedBinderRegistry();

  service_manager::BinderRegistryWithArgs<RenderProcessHost*,
                                          const url::Origin&>
      parameterized_binder_registry_;
  service_manager::BinderRegistry binder_registry_;
};

void WorkerInterfaceBinders::InitializeParameterizedBinderRegistry() {
  parameterized_binder_registry_.AddInterface(
      base::Bind(&PermissionServiceContext::CreateServiceForWorker));
}

}  // namespace

void BindWorkerInterface(const std::string& interface_name,
                         mojo::ScopedMessagePipeHandle interface_pipe,
                         RenderProcessHost* host,
                         const url::Origin& origin) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  CR_DEFINE_STATIC_LOCAL(WorkerInterfaceBinders, binders, ());
  binders.BindInterface(interface_name, std::move(interface_pipe), host,
                        origin);
}

}  // namespace content
