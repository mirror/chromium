// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PRINTING_SERVICE_PUBLIC_CPP_PDF_COMPOSITOR_CLIENT_H_
#define COMPONENTS_PRINTING_SERVICE_PUBLIC_CPP_PDF_COMPOSITOR_CLIENT_H_

#include "base/memory/shared_memory_handle.h"
#include "components/printing/service/public/interfaces/pdf_compositor.mojom.h"
#include "services/service_manager/public/cpp/connector.h"

namespace printing {

// Helper class to composite a pdf via the pdf_compositor service.
class PdfCompositorClient {
 public:
  PdfCompositorClient();
  ~PdfCompositorClient();

  // Add the map between a frame's id and its content uid.
  void AddSubframeMap(service_manager::Connector* connector,
                      int frame_id,
                      uint32_t uid);

  // Add a frame's content and its out-of-process subframe ids.
  void AddSubframeContent(service_manager::Connector* connector,
                          int frame_id,
                          base::SharedMemoryHandle handle,
                          size_t data_size,
                          const std::vector<uint32_t>& subframe_content_ids);

  // Check whether it is ready to composite this frame.
  void IsReadyToComposite(
      service_manager::Connector* connector,
      int frame_id,
      uint32_t page_num,
      const std::vector<uint32_t>& subframe_content_ids,
      mojom::PdfCompositor::IsReadyToCompositeCallback callback,
      scoped_refptr<base::SequencedTaskRunner> callback_task_runner);

  // Composite the final picture and convert into a PDF file.
  void CompositeToPdf(
      service_manager::Connector* connector,
      int frame_id,
      uint32_t page_num,
      base::SharedMemoryHandle handle,
      size_t data_size,
      const std::vector<uint32_t>& subframe_content_ids,
      mojom::PdfCompositor::CompositeToPdfCallback callback,
      scoped_refptr<base::SequencedTaskRunner> callback_task_runner);

 private:
  // Connect to the service.
  void Connect(service_manager::Connector* connector);

  mojom::PdfCompositorPtr compositor_;

  DISALLOW_COPY_AND_ASSIGN(PdfCompositorClient);
};

}  // namespace printing

#endif  // COMPONENTS_PRINTING_SERVICE_PUBLIC_CPP_PDF_COMPOSITOR_CLIENT_H_
