// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/internal/blob_task_proxy.h"

#include "base/guid.h"
#include "base/task_runner_util.h"
#include "storage/browser/blob/blob_data_builder.h"
#include "storage/browser/blob/blob_data_handle.h"
#include "storage/browser/blob/blob_storage_context.h"

namespace download {

BlobTaskProxy::BlobTaskProxy(
    base::WeakPtr<storage::BlobStorageContext> blob_storage_context)
    : main_task_runner_(base::ThreadTaskRunnerHandle::Get()),
      weak_ptr_factory_(this) {}

BlobTaskProxy::~BlobTaskProxy() {}

// Save blob data on IO thread. |callback| will be called on main thread after
// blob data construction complete.
void BlobTaskProxy::SaveAsBlobOnIO(std::unique_ptr<std::string> data,
                                   BlobDataHandleCallback callback) {
  // Build blob data.
  std::string blob_uuid = base::GenerateGUID();
  storage::BlobDataBuilder builder(blob_uuid);
  builder.AppendData(*data.get());
  blob_data_handle_ = blob_storage_context_->AddFinishedBlob(builder);

  // Wait for blob data construction complete.
  auto cb = base::BindRepeating(&BlobTaskProxy::BlobSavedOnIO,
                                weak_ptr_factory_.GetWeakPtr(),
                                base::Passed(std::move(callback)));
  blob_data_handle_->RunOnConstructionComplete(cb);
}

void BlobTaskProxy::BlobSavedOnIO(BlobDataHandleCallback callback,
                                  storage::BlobStatus status) {
  // Relay BlobDataHandle back to main thread.
  auto cb = base::BindOnce(std::move(callback),
                           base::Passed(std::move(blob_data_handle_)));
  main_task_runner_->PostTask(FROM_HERE, std::move(cb));
}

}  // namespace download
