// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/printing/browser/print_composite_client.h"

#include <memory>
#include <utility>

#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/common/service_manager_connection.h"
#include "mojo/public/cpp/system/platform_handle.h"
#include "services/service_manager/public/cpp/connector.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(printing::PrintCompositeClient);

namespace printing {

PrintCompositeClient::PrintCompositeClient(content::WebContents* web_contents)
    : for_preview_(false), weak_factory_(this) {}

PrintCompositeClient::~PrintCompositeClient() {}

void PrintCompositeClient::CreateConnectorRequest() {
  connector_ = service_manager::Connector::Create(&connector_request_);
  content::ServiceManagerConnection::GetForProcess()
      ->GetConnector()
      ->BindConnectorRequest(std::move(connector_request_));
}

void PrintCompositeClient::DoCompositeToPdf(
    uint32_t frame_id,
    uint32_t page_num,
    base::SharedMemoryHandle handle,
    uint32_t data_size,
    std::vector<uint32_t> subframe_uids,
    mojom::PdfCompositor::CompositeToPdfCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (!connector_)
    CreateConnectorRequest();

  // Check whether it is ok to composite, if so, composite to PDF.
  IsReadyToComposite(
      connector_.get(), frame_id, page_num, subframe_uids,
      base::BindOnce(&PrintCompositeClient::OnIsReadyToComposite,
                     weak_factory_.GetWeakPtr(), frame_id, page_num, handle,
                     data_size, subframe_uids, std::move(callback)),
      base::ThreadTaskRunnerHandle::Get());
}

void PrintCompositeClient::OnIsReadyToComposite(
    int frame_id,
    uint32_t page_num,
    base::SharedMemoryHandle handle,
    uint32_t data_size,
    std::vector<uint32_t> subframe_uids,
    mojom::PdfCompositor::CompositeToPdfCallback callback,
    bool status) {
  if (status) {
    // Send to composite.
    CompositeToPdf(connector_.get(), frame_id, page_num, handle, data_size,
                   subframe_uids, std::move(callback),
                   base::ThreadTaskRunnerHandle::Get());
  } else {
    base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE,
        base::BindOnce(&PrintCompositeClient::DoCompositeToPdf,
                       weak_factory_.GetWeakPtr(), frame_id, page_num, handle,
                       data_size, subframe_uids, std::move(callback)),
        base::TimeDelta::FromMilliseconds(100));
  }
}

std::unique_ptr<base::SharedMemory> PrintCompositeClient::GetShmFromMojoHandle(
    mojo::ScopedSharedBufferHandle handle) {
  base::SharedMemoryHandle memory_handle;
  size_t memory_size = 0;
  bool read_only_flag = false;

  const MojoResult result = mojo::UnwrapSharedMemoryHandle(
      std::move(handle), &memory_handle, &memory_size, &read_only_flag);
  if (result != MOJO_RESULT_OK)
    return nullptr;
  DCHECK_GT(memory_size, 0u);

  std::unique_ptr<base::SharedMemory> shm =
      base::MakeUnique<base::SharedMemory>(memory_handle, true /* read_only */);
  if (!shm->Map(memory_size)) {
    DLOG(ERROR) << "Map shared memory failed.";
    return nullptr;
  }
  return shm;
}

scoped_refptr<base::RefCountedBytes>
PrintCompositeClient::GetDataFromMojoHandle(
    mojo::ScopedSharedBufferHandle handle) {
  std::unique_ptr<base::SharedMemory> shm =
      GetShmFromMojoHandle(std::move(handle));
  if (!shm)
    return nullptr;

  return base::MakeRefCounted<base::RefCountedBytes>(
      reinterpret_cast<const unsigned char*>(shm->memory()),
      shm->mapped_size());
}

}  // namespace printing
