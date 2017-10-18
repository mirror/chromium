// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/worker_interface_filtering.h"

#include <utility>

#include "content/public/browser/browser_context.h"
#include "content/public/browser/render_process_host.h"
#include "services/service_manager/public/cpp/connector.h"

namespace content {

void FilterInterfacesForWorker(
    base::StringPiece spec,
    int process_id,
    service_manager::mojom::InterfaceProviderRequest request,
    service_manager::mojom::InterfaceProviderPtr provider) {
  auto* process = RenderProcessHost::FromID(process_id);
  if (!process)
    return;

  service_manager::Connector* connector =
      BrowserContext::GetConnectorFor(process->GetBrowserContext());
  // |connector| is null in unit tests.
  if (!connector)
    return;

  connector->FilterInterfaces(spec.as_string(), process->GetChildIdentity(),
                              std::move(request), std::move(provider));
}

}  // namespace content
