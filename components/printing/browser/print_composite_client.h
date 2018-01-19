// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PRINTING_BROWSER_PRINT_COMPOSITE_CLIENT_H_
#define COMPONENTS_PRINTING_BROWSER_PRINT_COMPOSITE_CLIENT_H_

#include <map>
#include <memory>

#include "base/containers/flat_map.h"
#include "components/printing/service/public/interfaces/pdf_compositor.mojom.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "services/service_manager/public/cpp/connector.h"

namespace printing {

// Class to manage print requests and their communication with pdf
// compositor service.
// Print preview always print indivial pages and then the complete
// document. In this case, we store the interface pointer in a map to be
// reused among these serial requests.
// Basic print always print the entire document, so we will simply
// create a new interface pointer every time.
class PrintCompositeClient
    : public content::WebContentsUserData<PrintCompositeClient> {
 public:
  using ContentToFrameMap = std::unordered_map<uint32_t, uint64_t>;

  explicit PrintCompositeClient(content::WebContents* web_contents);
  ~PrintCompositeClient() override;

  // NOTE: |handle| must be a READ-ONLY base::SharedMemoryHandle, i.e. one
  // acquired by base::SharedMemory::GetReadOnlyHandle().
  void DoCompositePageToPdf(
      int cookie,
      uint64_t frame_guid,
      uint32_t page_num,
      base::SharedMemoryHandle handle,
      uint32_t data_size,
      ContentToFrameMap subframe_content_map,
      mojom::PdfCompositor::CompositePageToPdfCallback callback);

  void DoCompositeDocumentToPdf(
      int cookie,
      uint64_t frame_guid,
      base::SharedMemoryHandle handle,
      uint32_t data_size,
      ContentToFrameMap subframe_content_map,
      mojom::PdfCompositor::CompositeDocumentToPdfCallback callback);

  void set_for_preview(bool for_preview) { for_preview_ = for_preview; }
  bool for_preview() const { return for_preview_; }

 private:
  // Get the request, but doesn't own it.
  mojom::PdfCompositorPtr& GetCompositeRequest(int cookie);

  // Find an existing request or create a new one, and own it.
  mojom::PdfCompositorPtr OwnCompositeRequest(int cookie);

  mojom::PdfCompositorPtr CreateCompositeRequest();

  // Whether this client is created for print preview dialog.
  bool for_preview_;

  std::unique_ptr<service_manager::Connector> connector_;

  // Stores the mapping between document cookies and their corresponding
  // requests. A document cookie is unique for each print query, thus can be a
  // key for print requests.
  base::flat_map<int, mojom::PdfCompositorPtr> compositor_map_;

  DISALLOW_COPY_AND_ASSIGN(PrintCompositeClient);
};

}  // namespace printing

#endif  // COMPONENTS_PRINTING_BROWSER_PRINT_COMPOSITE_CLIENT_H_
