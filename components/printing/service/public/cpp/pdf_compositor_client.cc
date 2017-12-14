// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/printing/service/public/cpp/pdf_compositor_client.h"

#include <utility>

#include "mojo/public/cpp/system/platform_handle.h"

namespace printing {

namespace {

// Helper callback which owns an PdfCompositorPtr until invoked. This keeps the
// PdfCompositor pipe open just long enough to dispatch a reply, at which point
// the reply is forwarded to the wrapped |callback|.
void OnCompositeToPdf(
    printing::mojom::PdfCompositor::CompositeToPdfCallback callback,
    scoped_refptr<base::SequencedTaskRunner> task_runner,
    mojom::PdfCompositor::Status status,
    mojo::ScopedSharedBufferHandle pdf_handle) {
  task_runner->PostTask(FROM_HERE, base::BindOnce(std::move(callback), status,
                                                  base::Passed(&pdf_handle)));
}

void OnIsReadyToComposite(
    printing::mojom::PdfCompositor::IsReadyToCompositeCallback callback,
    scoped_refptr<base::SequencedTaskRunner> task_runner,
    bool is_ready) {
  task_runner->PostTask(FROM_HERE,
                        base::BindOnce(std::move(callback), is_ready));
}

}  // namespace

PdfCompositorClient::PdfCompositorClient() : compositor_(nullptr) {}

PdfCompositorClient::~PdfCompositorClient() {}

void PdfCompositorClient::Connect(service_manager::Connector* connector) {
  DCHECK(!compositor_.is_bound());
  connector->BindInterface(mojom::kServiceName, &compositor_);
}

void PdfCompositorClient::AddSubframeMap(service_manager::Connector* connector,
                                         int frame_id,
                                         uint32_t content_id) {
  if (!compositor_)
    Connect(connector);
  compositor_->AddSubframeMap(frame_id, content_id);
}

void PdfCompositorClient::AddSubframeContent(
    service_manager::Connector* connector,
    int frame_id,
    base::SharedMemoryHandle handle,
    size_t data_size,
    const std::vector<uint32_t>& subframe_content_ids) {
  if (!compositor_)
    Connect(connector);

  DCHECK(data_size);
  mojo::ScopedSharedBufferHandle buffer_handle =
      mojo::WrapSharedMemoryHandle(handle, data_size, true);

  compositor_->AddSubframeContent(frame_id, std::move(buffer_handle),
                                  subframe_content_ids);
}

// Check whether it is time to composite this frame.
void PdfCompositorClient::IsReadyToComposite(
    service_manager::Connector* connector,
    int frame_id,
    uint32_t page_num,
    const std::vector<uint32_t>& subframe_content_ids,
    mojom::PdfCompositor::IsReadyToCompositeCallback callback,
    scoped_refptr<base::SequencedTaskRunner> callback_task_runner) {
  if (!compositor_)
    Connect(connector);

  compositor_->IsReadyToComposite(
      frame_id, page_num, subframe_content_ids,
      base::BindOnce(&OnIsReadyToComposite, std::move(callback),
                     callback_task_runner));
}

// Composite the final picture and convert into a PDF file.
void PdfCompositorClient::CompositeToPdf(
    service_manager::Connector* connector,
    int frame_id,
    uint32_t page_num,
    base::SharedMemoryHandle handle,
    size_t data_size,
    const std::vector<uint32_t>& subframe_content_ids,
    mojom::PdfCompositor::CompositeToPdfCallback callback,
    scoped_refptr<base::SequencedTaskRunner> callback_task_runner) {
  if (!compositor_)
    Connect(connector);

  DCHECK(data_size);
  mojo::ScopedSharedBufferHandle buffer_handle =
      mojo::WrapSharedMemoryHandle(handle, data_size, true);

  compositor_->CompositeToPdf(
      frame_id, page_num, std::move(buffer_handle), subframe_content_ids,
      base::BindOnce(&OnCompositeToPdf, std::move(callback),
                     callback_task_runner));
}

}  // namespace printing
