// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/printing/browser/print_composite_client.h"

#include <utility>

#include "base/bind.h"
#include "base/memory/shared_memory_handle.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/service_manager_connection.h"
#include "mojo/public/cpp/system/platform_handle.h"
#include "services/service_manager/public/cpp/connector.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(printing::PrintCompositeClient);

namespace {
void OnDidCompositeDocumentToPdf(
    printing::mojom::PdfCompositorPtr compositor,
    printing::mojom::PdfCompositor::CompositeDocumentToPdfCallback callback,
    printing::mojom::PdfCompositor::Status status,
    mojo::ScopedSharedBufferHandle handle) {
  std::move(callback).Run(status, std::move(handle));
}
}  // namespace

namespace printing {

PrintCompositeClient::PrintCompositeClient(content::WebContents* web_contents)
    : for_preview_(false) {}

PrintCompositeClient::~PrintCompositeClient() {}

void PrintCompositeClient::DoCompositePageToPdf(
    int document_cookie,
    uint64_t frame_guid,
    uint32_t page_num,
    base::SharedMemoryHandle handle,
    uint32_t data_size,
    ContentToFrameMap subframe_content_map,
    mojom::PdfCompositor::CompositePageToPdfCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  auto& compositor = GetCompositeRequest(document_cookie);

  DCHECK(data_size);
  mojo::ScopedSharedBufferHandle buffer_handle = mojo::WrapSharedMemoryHandle(
      handle, data_size,
      mojo::UnwrappedSharedMemoryHandleProtection::kReadOnly);
  compositor->CompositePageToPdf(frame_guid, page_num, std::move(buffer_handle),
                                 std::move(subframe_content_map),
                                 std::move(callback));
}

void PrintCompositeClient::DoCompositeDocumentToPdf(
    int document_cookie,
    uint64_t frame_guid,
    base::SharedMemoryHandle handle,
    uint32_t data_size,
    ContentToFrameMap subframe_content_map,
    mojom::PdfCompositor::CompositeDocumentToPdfCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  auto compositor = OwnCompositeRequest(document_cookie);

  DCHECK(data_size);
  mojo::ScopedSharedBufferHandle buffer_handle = mojo::WrapSharedMemoryHandle(
      handle, data_size,
      mojo::UnwrappedSharedMemoryHandleProtection::kReadOnly);
  auto call_once = base::AdaptCallbackForRepeating(std::move(callback));
  compositor->CompositeDocumentToPdf(
      frame_guid, std::move(buffer_handle), std::move(subframe_content_map),
      base::BindOnce(&OnDidCompositeDocumentToPdf, std::move(compositor),
                     std::move(call_once)));
}

mojom::PdfCompositorPtr& PrintCompositeClient::GetCompositeRequest(int cookie) {
  auto iter = compositor_map_.find(cookie);
  if (iter != compositor_map_.end()) {
    if (!iter->second.is_bound()) {
      NOTREACHED();
      iter->second = CreateCompositeRequest();
    }
    return iter->second;
  }

  compositor_map_[cookie] = CreateCompositeRequest();
  return compositor_map_[cookie];
}

mojom::PdfCompositorPtr PrintCompositeClient::OwnCompositeRequest(int cookie) {
  auto iter = compositor_map_.find(cookie);
  if (iter == compositor_map_.end()) {
    return CreateCompositeRequest();
  }

  auto compositor = std::move(iter->second);
  compositor_map_.erase(iter);
  return compositor;
}

mojom::PdfCompositorPtr PrintCompositeClient::CreateCompositeRequest() {
  if (!connector_) {
    service_manager::mojom::ConnectorRequest connector_request;
    connector_ = service_manager::Connector::Create(&connector_request);
    content::ServiceManagerConnection::GetForProcess()
        ->GetConnector()
        ->BindConnectorRequest(std::move(connector_request));
  }
  mojom::PdfCompositorPtr compositor;
  connector_->BindInterface(mojom::kServiceName, &compositor);
  return compositor;
}

}  // namespace printing
