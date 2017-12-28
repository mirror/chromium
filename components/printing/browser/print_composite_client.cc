// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/printing/browser/print_composite_client.h"

#include <memory>
#include <utility>

#include "components/printing/common/print_messages.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/common/service_manager_connection.h"
#include "printing/common/pdf_metafile_utils.h"
#include "services/service_manager/public/cpp/connector.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(printing::PrintCompositeClient);

namespace printing {

PrintCompositeClient::PrintCompositeClient(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      for_preview_(false),
      weak_factory_(this) {}

PrintCompositeClient::~PrintCompositeClient() {}

void PrintCompositeClient::CreateConnectorRequest() {
  connector_ = service_manager::Connector::Create(&connector_request_);
  content::ServiceManagerConnection::GetForProcess()
      ->GetConnector()
      ->BindConnectorRequest(std::move(connector_request_));
  Connect(connector_.get());
}

void PrintCompositeClient::PrintSubframe(const gfx::Rect& rect,
                                         uint64_t content_id,
                                         content::RenderFrameHost* dst_host) {
  int dst_frame_id = dst_host->GetRoutingID();

  if (!connector_)
    CreateConnectorRequest();
  // Store the mapping between content id and subframe id.
  AddSubframeMap(connector_.get(),
                 GenFrameGuid(dst_host->GetProcess()->GetID(), dst_frame_id),
                 content_id);

  // Check whether this subframe has been requested.
  auto result = pending_subframes_.insert(
      std::make_pair(dst_host->GetProcess()->GetID(), dst_frame_id));
  if (!result.second)
    return;

  // Send the request to remote frame.
  dst_host->Send(new PrintMsg_PrintFrameContent(dst_frame_id, rect));
}

bool PrintCompositeClient::OnMessageReceived(
    const IPC::Message& message,
    content::RenderFrameHost* render_frame_host) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP_WITH_PARAM(PrintCompositeClient, message,
                                   render_frame_host)
    IPC_MESSAGE_HANDLER(PrintHostMsg_DidPrintFrameContent,
                        OnDidPrintFrameContent)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void PrintCompositeClient::OnDidPrintFrameContent(
    content::RenderFrameHost* render_frame_host,
    const PrintHostMsg_DidPrintContent_Params& params) {
  DCHECK(connector_);

  AddSubframeContent(connector_.get(),
                     GenFrameGuid(render_frame_host->GetProcess()->GetID(),
                                  render_frame_host->GetRoutingID()),
                     params.metafile_data_handle, params.data_size,
                     params.subframe_content_ids);
}

void PrintCompositeClient::DoCompositeToPdf(
    uint64_t frame_guid,
    uint32_t page_num,
    base::SharedMemoryHandle handle,
    uint32_t data_size,
    const std::vector<uint64_t>& subframe_content_ids,
    mojom::PdfCompositor::CompositeToPdfCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (!connector_)
    CreateConnectorRequest();

  // Check whether it is ok to composite, if so, composite to PDF.
  DoCompositeToPdfImpl(frame_guid, page_num, handle, data_size,
                       subframe_content_ids, std::move(callback));
}

void PrintCompositeClient::DoCompositeToPdfImpl(
    uint64_t frame_guid,
    uint32_t page_num,
    base::SharedMemoryHandle handle,
    uint32_t data_size,
    const std::vector<uint64_t>& subframe_content_ids,
    mojom::PdfCompositor::CompositeToPdfCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  // Check whether it is ok to composite, if so, composite to PDF.
  IsReadyToComposite(
      connector_.get(), frame_guid, page_num, subframe_content_ids,
      base::BindOnce(&PrintCompositeClient::OnIsReadyToComposite,
                     weak_factory_.GetWeakPtr(), frame_guid, page_num, handle,
                     data_size, subframe_content_ids, std::move(callback)),
      base::ThreadTaskRunnerHandle::Get());
}

void PrintCompositeClient::OnIsReadyToComposite(
    uint64_t frame_guid,
    uint32_t page_num,
    base::SharedMemoryHandle handle,
    uint32_t data_size,
    const std::vector<uint64_t>& subframe_content_ids,
    mojom::PdfCompositor::CompositeToPdfCallback callback,
    bool is_ready) {
  if (is_ready) {
    // Send to composite.
    CompositeToPdf(connector_.get(), frame_guid, page_num, handle, data_size,
                   subframe_content_ids, std::move(callback),
                   base::ThreadTaskRunnerHandle::Get());
  } else {
    base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE,
        base::BindOnce(&PrintCompositeClient::DoCompositeToPdfImpl,
                       weak_factory_.GetWeakPtr(), frame_guid, page_num, handle,
                       data_size, subframe_content_ids, std::move(callback)),
        base::TimeDelta::FromMilliseconds(100));
  }
}

}  // namespace printing
