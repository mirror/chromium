// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/worker_binder_registry.h"

#include "content/browser/permissions/permission_service_context.h"
#include "content/browser/renderer_host/render_process_host_impl.h"
#include "content/public/browser/render_process_host.h"
#include "third_party/WebKit/public/platform/modules/permissions/permission.mojom.h"

namespace content {

BinderRegistryType& GetWorkerBinderRegistry() {
  CR_DEFINE_STATIC_LOCAL(std::unique_ptr<BinderRegistryType>, registry, ());
  if (!registry) {
    registry = base::MakeUnique<BinderRegistryType>();
    registry->AddInterface(
        base::Bind([](blink::mojom::PermissionServiceRequest request,
                      RenderProcessHost* host, const url::Origin& origin) {
          static_cast<RenderProcessHostImpl*>(host)
              ->permission_service_context()
              .CreateService(std::move(request), origin);
        }));
  }
  return *registry;
}
}  // namespace content
