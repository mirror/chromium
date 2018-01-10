// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PRINTING_BROWSER_PRINT_COMPOSITE_CLIENT_H_
#define COMPONENTS_PRINTING_BROWSER_PRINT_COMPOSITE_CLIENT_H_

#include "components/printing/service/public/interfaces/pdf_compositor.mojom.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

namespace printing {

class PrintCompositeClient
    : public content::WebContentsUserData<PrintCompositeClient> {
 public:
  explicit PrintCompositeClient(content::WebContents* web_contents);
  ~PrintCompositeClient() override;

  void DoCompositePageToPdf(
      uint64_t frame_guid,
      uint32_t page_num,
      base::SharedMemoryHandle handle,
      uint32_t data_size,
      const std::vector<uint32_t>& subframe_content_ids,
      mojom::PdfCompositor::CompositePageToPdfCallback callback);
  void DoCompositeDocumentToPdf(
      uint64_t frame_guid,
      base::SharedMemoryHandle handle,
      uint32_t data_size,
      const std::vector<uint32_t>& subframe_content_ids,
      mojom::PdfCompositor::CompositeDocumentToPdfCallback callback);

  void set_for_preview(bool for_preview) { for_preview_ = for_preview; }
  bool for_preview() const { return for_preview_; }

 private:
  void CreateServiceRequest();

  mojom::PdfCompositorPtr compositor_;
  bool for_preview_;

  DISALLOW_COPY_AND_ASSIGN(PrintCompositeClient);
};

}  // namespace printing

#endif  // COMPONENTS_PRINTING_BROWSER_PRINT_COMPOSITE_CLIENT_H_
