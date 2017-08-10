// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PRINTING_BROWSER_PRINT_COMPOSITE_CLIENT_H_
#define COMPONENTS_PRINTING_BROWSER_PRINT_COMPOSITE_CLIENT_H_

#include "base/memory/ref_counted_memory.h"
#include "base/memory/shared_memory.h"
#include "base/memory/shared_memory_handle.h"
#include "base/sequenced_task_runner.h"
#include "components/printing/service/public/cpp/pdf_compositor_client.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

namespace printing {

class PrintCompositeClient
    : public PdfCompositorClient,
      public content::WebContentsObserver,
      public content::WebContentsUserData<PrintCompositeClient> {
 public:
  explicit PrintCompositeClient(content::WebContents* web_contents);
  ~PrintCompositeClient() override;

  void CreateConnectorRequest();
  void DoComposite(
      base::SharedMemoryHandle handle,
      uint32_t data_size,
      mojom::PdfCompositor::CompositePdfCallback callback,
      scoped_refptr<base::SequencedTaskRunner> callback_task_runner);

  // Utility functions.
  static std::unique_ptr<base::SharedMemory> GetShmFromMojoHandle(
      mojo::ScopedSharedBufferHandle handle);
  static scoped_refptr<base::RefCountedBytes> GetDataFromMojoHandle(
      mojo::ScopedSharedBufferHandle handle);

 private:
  service_manager::mojom::ConnectorRequest connector_request_;
  std::unique_ptr<service_manager::Connector> connector_;
  base::WeakPtrFactory<PrintCompositeClient> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(PrintCompositeClient);
};

}  // namespace printing

#endif  // COMPONENTS_PRINTING_BROWSER_PRINT_COMPOSITE_CLIENT_H_
