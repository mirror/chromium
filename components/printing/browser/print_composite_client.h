// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PRINTING_BROWSER_PRINT_COMPOSITE_CLIENT_H_
#define COMPONENTS_PRINTING_BROWSER_PRINT_COMPOSITE_CLIENT_H_

#include "components/printing/service/public/cpp/pdf_compositor_client.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

namespace printing {

class PrintCompositeClient
    : public PdfCompositorClient,
      public content::WebContentsUserData<PrintCompositeClient> {
 public:
  explicit PrintCompositeClient(content::WebContents* web_contents);
  ~PrintCompositeClient() override;

  void DoCompositeToPdf(uint32_t frame_id,
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

  base::WeakPtrFactory<PrintCompositeClient> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(PrintCompositeClient);
};

}  // namespace printing

#endif  // COMPONENTS_PRINTING_BROWSER_PRINT_COMPOSITE_CLIENT_H_
