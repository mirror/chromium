// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PRINTING_BROWSER_PRINT_COMPOSITE_CLIENT_H_
#define COMPONENTS_PRINTING_BROWSER_PRINT_COMPOSITE_CLIENT_H_

#include <map>
#include <memory>

#include "base/optional.h"
#include "components/printing/service/public/cpp/pdf_service_mojo_types.h"
#include "components/printing/service/public/interfaces/pdf_compositor.mojom.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "printing/common/pdf_metafile_utils.h"
#include "services/service_manager/public/cpp/connector.h"

struct PrintHostMsg_DidPrintContent_Params;

namespace printing {

// Class to manage print requests and their communication with pdf
// compositor service.
// Each composite request have a separate interface pointer to connect
// with remote service. The request and its subframe printing results are
// tracked by its document cookie and print page number.
class PrintCompositeClient
    : public content::WebContentsUserData<PrintCompositeClient>,
      public content::WebContentsObserver {
 public:
  explicit PrintCompositeClient(content::WebContents* web_contents);
  ~PrintCompositeClient() override;

  // content::WebContentsObserver
  bool OnMessageReceived(const IPC::Message& message,
                         content::RenderFrameHost* render_frame_host) override;

  // IPC message handler.
  void OnDidPrintFrameContent(
      content::RenderFrameHost* render_frame_host,
      int document_cookie,
      int page_number,
      const PrintHostMsg_DidPrintContent_Params& params);

  // Instructs the specified subframe to print.
  void PrintSubframe(const gfx::Rect& rect,
                     int document_cookie,
                     int page_number,
                     content::RenderFrameHost* render_frame_host);

  // NOTE: |handle| must be a READ-ONLY base::SharedMemoryHandle, i.e. one
  // acquired by base::SharedMemory::GetReadOnlyHandle().
  void DoCompositePageToPdf(
      int cookie,
      uint64_t frame_guid,
      int page_num,
      base::SharedMemoryHandle handle,
      uint32_t data_size,
      const ContentToFrameMap& subframe_content_map,
      mojom::PdfCompositor::CompositePageToPdfCallback callback);

  void DoCompositeDocumentToPdf(
      int cookie,
      uint64_t frame_guid,
      base::SharedMemoryHandle handle,
      uint32_t data_size,
      const ContentToFrameMap& subframe_content_map,
      mojom::PdfCompositor::CompositeDocumentToPdfCallback callback);

  void set_for_preview(bool for_preview) { for_preview_ = for_preview; }
  bool for_preview() const { return for_preview_; }

  // Convert a content id to proxy id map to a content id to frame's global
  // unique id map.
  static ContentToFrameMap ConvertContentInfoMap(
      content::WebContents* web_contents,
      content::RenderFrameHost* render_frame_host,
      const ContentToProxyIdMap& content_proxy_map);

 private:
  // Since page number is always non-negative, use this value to indicate it is
  // for the whole document -- no page number specified.
  static constexpr int kPageNumForWholeDoc = -1;

  // Callback functions for getting the replies.
  void OnDidCompositePageToPdf(
      int page_num,
      int document_cookie,
      printing::mojom::PdfCompositor::CompositePageToPdfCallback callback,
      printing::mojom::PdfCompositor::Status status,
      mojo::ScopedSharedBufferHandle handle);

  void OnDidCompositeDocumentToPdf(
      int document_cookie,
      printing::mojom::PdfCompositor::CompositeDocumentToPdfCallback callback,
      printing::mojom::PdfCompositor::Status status,
      mojo::ScopedSharedBufferHandle handle);

  // Get the request, but doesn't own it.
  mojom::PdfCompositorPtr& GetCompositeRequest(int cookie,
                                               base::Optional<int> page_num);

  // Find an existing request or create a new one, and own it.
  void RemoveCompositeRequest(int cookie, base::Optional<int> page_num);

  mojom::PdfCompositorPtr CreateCompositeRequest();

  // Whether this client is created for print preview dialog.
  bool for_preview_;

  std::unique_ptr<service_manager::Connector> connector_;

  // Stores the mapping between <document cookie, page number> and their
  // corresponding requests.
  std::map<std::pair<int, int>, mojom::PdfCompositorPtr> compositor_map_;

  DISALLOW_COPY_AND_ASSIGN(PrintCompositeClient);
};

}  // namespace printing

#endif  // COMPONENTS_PRINTING_BROWSER_PRINT_COMPOSITE_CLIENT_H_
