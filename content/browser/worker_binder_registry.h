// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WORKER_BINDER_REGISTRY_H_
#define CONTENT_BROWSER_WORKER_BINDER_REGISTRY_H_

#include "services/service_manager/public/cpp/binder_registry.h"
#include "url/origin.h"

namespace content {
class RenderProcessHost;

using BinderRegistryType =
    service_manager::BinderRegistryWithArgs<RenderProcessHost*,
                                            const url::Origin&>;

BinderRegistryType& GetWorkerBinderRegistry();

}  // namespace content

#endif  // CONTENT_BROWSER_WORKER_BINDER_REGISTRY_H_
