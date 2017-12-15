// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DOWNLOAD_INTERNAL_BLOB_TASK_PROXY_H_
#define COMPONENTS_DOWNLOAD_INTERNAL_BLOB_TASK_PROXY_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/single_thread_task_runner.h"
#include "storage/common/blob_storage/blob_storage_constants.h"

namespace storage {
class BlobDataHandle;
class BlobStorageContext;
}  // namespace storage

namespace download {

// Proxy for blob related task on IO thread.
// Created on main thread and do work on IO thread, destroyed on IO thread.
class BlobTaskProxy {
 public:
  using BlobDataHandleCallback =
      base::OnceCallback<void(std::unique_ptr<storage::BlobDataHandle>)>;

  BlobTaskProxy(
      base::WeakPtr<storage::BlobStorageContext> blob_storage_context);
  ~BlobTaskProxy();

  // Save blob data on IO thread. |callback| will be called on main thread after
  // blob construction completes.
  void SaveAsBlobOnIO(std::unique_ptr<std::string> data,
                      BlobDataHandleCallback callback);

 private:
  void BlobSavedOnIO(BlobDataHandleCallback callback,
                     storage::BlobStatus status);

  scoped_refptr<base::SingleThreadTaskRunner> main_task_runner_;

  // Used to build blob data, must accessed on IO thread.
  base::WeakPtr<storage::BlobStorageContext> blob_storage_context_;

  // BlobDataHandle that will be eventually passed to main thread.
  std::unique_ptr<storage::BlobDataHandle> blob_data_handle_;

  // Bounded to IO thread task runner.
  base::WeakPtrFactory<BlobTaskProxy> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(BlobTaskProxy);
};

}  // namespace download

#endif  // COMPONENTS_DOWNLOAD_INTERNAL_BLOB_TASK_PROXY_H_
