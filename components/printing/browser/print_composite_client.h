// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PRINTING_BROWSER_PRINT_COMPOSITE_CLIENT_H_
#define COMPONENTS_PRINTING_BROWSER_PRINT_COMPOSITE_CLIENT_H_

#include "base/memory/weak_ptr.h"
#include "components/printing/service/public/cpp/pdf_compositor_client.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

struct PrintHostMsg_DidPrintContent_Params;

namespace printing {

class PrintCompositeClient
    : public PdfCompositorClient,
      public content::WebContentsUserData<PrintCompositeClient>,
      public content::WebContentsObserver {
 public:
  explicit PrintCompositeClient(content::WebContents* web_contents);
  ~PrintCompositeClient() override;

  // content::WebContentsObserver
  void PrintChildFrame(content::RenderFrameHost* dst_host,
                       const gfx::Rect& rect,
                       uint32_t content_id) override;
  bool OnMessageReceived(const IPC::Message& message,
                         content::RenderFrameHost* render_frame_host) override;

  // IPC message handler.
  void OnDidPrintFrameContent(
      content::RenderFrameHost* render_frame_host,
      const PrintHostMsg_DidPrintContent_Params& params);

  void DoCompositeToPdf(int frame_id,
                        uint32_t page_num,
                        base::SharedMemoryHandle handle,
                        uint32_t data_size,
                        const std::vector<uint32_t>& subframe_content_ids,
                        mojom::PdfCompositor::CompositeToPdfCallback callback);

  void set_for_preview(bool for_preview) { for_preview_ = for_preview; }
  bool for_preview() const { return for_preview_; }

 private:
  void CreateConnectorRequest();
  void OnIsReadyToComposite(
      int frame_id,
      uint32_t page_num,
      base::SharedMemoryHandle handle,
      uint32_t data_size,
      const std::vector<uint32_t>& subframe_content_ids,
      mojom::PdfCompositor::CompositeToPdfCallback callback,
      bool is_ready);

  service_manager::mojom::ConnectorRequest connector_request_;
  std::unique_ptr<service_manager::Connector> connector_;
  bool for_preview_;

  // All the subframes that have been requested for printing.
  std::set<int> pending_subframe_ids_;

  base::WeakPtrFactory<PrintCompositeClient> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(PrintCompositeClient);
};

}  // namespace printing

#endif  // COMPONENTS_PRINTING_BROWSER_PRINT_COMPOSITE_CLIENT_H_
