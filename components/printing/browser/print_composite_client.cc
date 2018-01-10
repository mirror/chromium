// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/printing/browser/print_composite_client.h"

#include <memory>
#include <utility>

#include "base/memory/shared_memory_handle.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/service_manager_connection.h"
#include "mojo/public/cpp/system/platform_handle.h"
#include "services/service_manager/public/cpp/connector.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(printing::PrintCompositeClient);

namespace printing {

PrintCompositeClient::PrintCompositeClient(content::WebContents* web_contents)
    : for_preview_(false) {}

PrintCompositeClient::~PrintCompositeClient() {}

void PrintCompositeClient::DoCompositePageToPdf(
    uint64_t frame_guid,
    uint32_t page_num,
    base::SharedMemoryHandle handle,
    uint32_t data_size,
    const std::vector<uint32_t>& subframe_content_ids,
    mojom::PdfCompositor::CompositePageToPdfCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (!compositor_.is_bound())
    CreateServiceRequest();

  DCHECK(data_size);
  mojo::ScopedSharedBufferHandle buffer_handle =
      mojo::WrapSharedMemoryHandle(handle, data_size, true);
  compositor_->CompositePageToPdf(frame_guid, page_num,
                                  std::move(buffer_handle),
                                  subframe_content_ids, std::move(callback));
}

void PrintCompositeClient::DoCompositeDocumentToPdf(
    uint64_t frame_guid,
    base::SharedMemoryHandle handle,
    uint32_t data_size,
    const std::vector<uint32_t>& subframe_content_ids,
    mojom::PdfCompositor::CompositeDocumentToPdfCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (!compositor_.is_bound())
    CreateServiceRequest();

  DCHECK(data_size);
  mojo::ScopedSharedBufferHandle buffer_handle =
      mojo::WrapSharedMemoryHandle(handle, data_size, true);
  compositor_->CompositeDocumentToPdf(frame_guid, std::move(buffer_handle),
                                      subframe_content_ids,
                                      std::move(callback));
}

void PrintCompositeClient::CreateServiceRequest() {
  service_manager::mojom::ConnectorRequest connector_request_;
  auto connector_ = service_manager::Connector::Create(&connector_request_);
  content::ServiceManagerConnection::GetForProcess()
      ->GetConnector()
      ->BindConnectorRequest(std::move(connector_request_));
  connector_->BindInterface(mojom::kServiceName, &compositor_);
}

}  // namespace printing
