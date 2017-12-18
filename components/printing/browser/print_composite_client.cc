// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/printing/browser/print_composite_client.h"

#include <memory>
#include <utility>

#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/common/service_manager_connection.h"
#include "services/service_manager/public/cpp/connector.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(printing::PrintCompositeClient);

namespace printing {

PrintCompositeClient::PrintCompositeClient(content::WebContents* web_contents)
    : for_preview_(false), weak_factory_(this) {}

PrintCompositeClient::~PrintCompositeClient() {}

void PrintCompositeClient::CreateConnectorRequest() {
  connector_ = service_manager::Connector::Create(&connector_request_);
  content::ServiceManagerConnection::GetForProcess()
      ->GetConnector()
      ->BindConnectorRequest(std::move(connector_request_));
}

void PrintCompositeClient::DoCompositeToPdf(
    int frame_id,
    uint32_t page_num,
    base::SharedMemoryHandle handle,
    uint32_t data_size,
    const std::vector<uint32_t>& subframe_content_ids,
    mojom::PdfCompositor::CompositeToPdfCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (!connector_)
    CreateConnectorRequest();

  // Check whether it is ok to composite, if so, composite to PDF.
  IsReadyToComposite(
      connector_.get(), frame_id, page_num, subframe_content_ids,
      base::BindOnce(&PrintCompositeClient::OnIsReadyToComposite,
                     weak_factory_.GetWeakPtr(), frame_id, page_num, handle,
                     data_size, subframe_content_ids, std::move(callback)),
      base::ThreadTaskRunnerHandle::Get());
}

void PrintCompositeClient::OnIsReadyToComposite(
    int frame_id,
    uint32_t page_num,
    base::SharedMemoryHandle handle,
    uint32_t data_size,
    const std::vector<uint32_t>& subframe_content_ids,
    mojom::PdfCompositor::CompositeToPdfCallback callback,
    bool is_ready) {
  if (is_ready) {
    // Send to composite.
    CompositeToPdf(connector_.get(), frame_id, page_num, handle, data_size,
                   subframe_content_ids, std::move(callback),
                   base::ThreadTaskRunnerHandle::Get());
  } else {
    base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE,
        base::BindOnce(&PrintCompositeClient::DoCompositeToPdf,
                       weak_factory_.GetWeakPtr(), frame_id, page_num, handle,
                       data_size, subframe_content_ids, std::move(callback)),
        base::TimeDelta::FromMilliseconds(100));
  }
}

}  // namespace printing
