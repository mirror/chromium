// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/media/service_connector_provider_impl.h"

#include "content/public/browser/browser_thread.h"
#include "content/public/common/service_manager_connection.h"
#include "media/capture/video/video_capture_gpu_jpeg_decoder.h"
#include "services/ui/public/cpp/gpu/gpu.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/video_capture/public/interfaces/constants.mojom.h"

namespace {

void BindToBrowserConnector(service_manager::mojom::ConnectorRequest request) {
  if (!content::BrowserThread::CurrentlyOn(content::BrowserThread::UI)) {
    content::BrowserThread::PostTask(
        content::BrowserThread::UI, FROM_HERE,
        base::Bind(&BindToBrowserConnector, base::Passed(&request)));
    return;
  }
  content::ServiceManagerConnection::GetForProcess()->GetConnector()
      ->BindConnectorRequest(std::move(request));
}

} // anonymous namespace

namespace content {

void ServiceConnectorProviderImpl::GetConnectorAsync(
    base::Callback<void(service_manager::Connector*)> callback) {
  if (!content::ServiceManagerConnection::GetForProcess()) {
    // This happens in unit tests that do not have Mojo enabled.
    DLOG(WARNING) << "Unable to connect to Gpu process, because no "
                  << "ServiceManagerConnection is available.";
    callback.Run(nullptr);
    return;
  }

  service_manager::mojom::ConnectorRequest connector_request;
  std::unique_ptr<service_manager::Connector> connector =
      service_manager::Connector::Create(&connector_request);
  BindToBrowserConnector(std::move(connector_request));
  callback.Run(connector.get());
}

}  // namespace content
