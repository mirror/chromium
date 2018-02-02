// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/printing/browser/print_composite_client.h"

#include <utility>

#include "base/bind.h"
#include "base/memory/shared_memory_handle.h"
#include "components/printing/common/print_messages.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/common/service_manager_connection.h"
#include "mojo/public/cpp/system/platform_handle.h"
#include "printing/printing_utils.h"
#include "services/service_manager/public/cpp/connector.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(printing::PrintCompositeClient);

namespace printing {

PrintCompositeClient::PrintCompositeClient(content::WebContents* web_contents)
    : for_preview_(false) {
  DCHECK(web_contents);
}

PrintCompositeClient::~PrintCompositeClient() {}

void PrintCompositeClient::OnDidPrintFrameContent(
    content::RenderFrameHost* render_frame_host,
    int document_cookie,
    int page_number,
    const PrintHostMsg_DidPrintContent_Params& params) {
  // Content in |params| is sent from untrusted source, only minimal processing
  // is done here. Most of it will be directly forwarded to pdf compositor
  // service.
  auto& compositor = GetCompositeRequest(document_cookie, page_number);
  DCHECK(compositor.is_bound());

  mojo::ScopedSharedBufferHandle buffer_handle = mojo::WrapSharedMemoryHandle(
      params.metafile_data_handle, params.data_size,
      mojo::UnwrappedSharedMemoryHandleProtection::kReadOnly);
  auto frame_guid = GenFrameGuid(render_frame_host->GetProcess()->GetID(),
                                 render_frame_host->GetRoutingID());
  compositor->AddSubframeContent(
      frame_guid, std::move(buffer_handle),
      ConvertContentInfoMap(web_contents(), render_frame_host,
                            params.subframe_content_info));
}

void PrintCompositeClient::PrintSubframe(
    const gfx::Rect& rect,
    int document_cookie,
    int page_number,
    content::RenderFrameHost* render_frame_host) {
  PrintMsg_PrintFrame_Params params;
  params.printable_area = rect;
  params.document_cookie = document_cookie;
  params.page_number = page_number;
  // Send the request to the destination frame.
  render_frame_host->Send(new PrintMsg_PrintFrameContent(
      render_frame_host->GetRoutingID(), params));
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

void PrintCompositeClient::DoCompositePageToPdf(
    int document_cookie,
    uint64_t frame_guid,
    int page_num,
    base::SharedMemoryHandle handle,
    uint32_t data_size,
    const ContentToFrameMap& subframe_content_map,
    mojom::PdfCompositor::CompositePageToPdfCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  auto& compositor = GetCompositeRequest(document_cookie, page_num);

  DCHECK(data_size);
  mojo::ScopedSharedBufferHandle buffer_handle = mojo::WrapSharedMemoryHandle(
      handle, data_size,
      mojo::UnwrappedSharedMemoryHandleProtection::kReadOnly);
  // Since this class owns compositor, compositor will be gone when this class
  // is destructed. Mojo won't call its callback in that case so it is safe to
  // use unretained |this| pointer here.
  compositor->CompositePageToPdf(
      frame_guid, page_num, std::move(buffer_handle), subframe_content_map,
      base::BindOnce(&PrintCompositeClient::OnDidCompositePageToPdf,
                     base::Unretained(this), page_num, document_cookie,
                     std::move(callback)));
}

void PrintCompositeClient::DoCompositeDocumentToPdf(
    int document_cookie,
    uint64_t frame_guid,
    base::SharedMemoryHandle handle,
    uint32_t data_size,
    const ContentToFrameMap& subframe_content_map,
    mojom::PdfCompositor::CompositeDocumentToPdfCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  auto& compositor = GetCompositeRequest(document_cookie, base::nullopt);

  DCHECK(data_size);
  mojo::ScopedSharedBufferHandle buffer_handle = mojo::WrapSharedMemoryHandle(
      handle, data_size,
      mojo::UnwrappedSharedMemoryHandleProtection::kReadOnly);
  // Since this class owns compositor, compositor will be gone when this class
  // is destructed. Mojo won't call its callback in that case so it is safe to
  // use unretained |this| pointer here.
  compositor->CompositeDocumentToPdf(
      frame_guid, std::move(buffer_handle), subframe_content_map,
      base::BindOnce(&PrintCompositeClient::OnDidCompositeDocumentToPdf,
                     base::Unretained(this), document_cookie,
                     std::move(callback)));
}

void PrintCompositeClient::OnDidCompositePageToPdf(
    int page_num,
    int document_cookie,
    printing::mojom::PdfCompositor::CompositePageToPdfCallback callback,
    printing::mojom::PdfCompositor::Status status,
    mojo::ScopedSharedBufferHandle handle) {
  RemoveCompositeRequest(document_cookie, page_num);
  std::move(callback).Run(status, std::move(handle));
}

void PrintCompositeClient::OnDidCompositeDocumentToPdf(
    int document_cookie,
    printing::mojom::PdfCompositor::CompositeDocumentToPdfCallback callback,
    printing::mojom::PdfCompositor::Status status,
    mojo::ScopedSharedBufferHandle handle) {
  RemoveCompositeRequest(document_cookie, base::nullopt);
  std::move(callback).Run(status, std::move(handle));
}

ContentToFrameMap PrintCompositeClient::ConvertContentInfoMap(
    content::WebContents* web_contents,
    content::RenderFrameHost* render_frame_host,
    const ContentToProxyIdMap& content_proxy_map) {
  ContentToFrameMap content_frame_map;
  int process_id = render_frame_host->GetProcess()->GetID();
  for (auto& entry : content_proxy_map) {
    auto content_id = entry.first;
    auto proxy_id = entry.second;
    // Find the render frame host which the render proxy id corresponds to.
    content::RenderFrameHost* rfh =
        web_contents->UnsafeFindFrameByFrameTreeNodeId(
            content::RenderFrameHost::GetFrameTreeNodeIdForRoutingId(process_id,
                                                                     proxy_id));
    if (!rfh) {
      // If we could not find the corresponding render frame host,
      // just skip it.
      continue;
    }

    // Store this frame's global unique id into the map.
    content_frame_map[content_id] =
        GenFrameGuid(rfh->GetProcess()->GetID(), rfh->GetRoutingID());
  }
  return content_frame_map;
}

mojom::PdfCompositorPtr& PrintCompositeClient::GetCompositeRequest(
    int cookie,
    base::Optional<int> page_num) {
  int page_no =
      page_num == base::nullopt ? kPageNumForWholeDoc : page_num.value();
  std::pair<int, int> key = std::make_pair(cookie, page_no);
  auto iter = compositor_map_.find(key);
  if (iter != compositor_map_.end())
    return iter->second;

  auto iterator = compositor_map_.emplace(key, CreateCompositeRequest()).first;
  return iterator->second;
}

void PrintCompositeClient::RemoveCompositeRequest(
    int cookie,
    base::Optional<int> page_num) {
  int page_no =
      page_num == base::nullopt ? kPageNumForWholeDoc : page_num.value();
  std::pair<int, int> key = std::make_pair(cookie, page_no);
  auto iter = compositor_map_.find(key);
  if (iter == compositor_map_.end())
    return;

  compositor_map_.erase(iter);
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
