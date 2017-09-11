// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_BLOB_STORAGE_REDIRECT_TO_BLOB_RESOURCE_HANDLER_H_
#define CONTENT_BROWSER_BLOB_STORAGE_REDIRECT_TO_BLOB_RESOURCE_HANDLER_H_

#include <stdint.h>
#include <memory>

#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "content/browser/loader/redirect_to_file_resource_handler.h"
#include "content/browser/loader/temporary_file_stream.h"
#include "content/common/content_export.h"
#include "storage/browser/blob/blob_data_handle.h"
#include "storage/browser/blob/blob_memory_controller.h"

namespace storage {
class BlobStorageContext;
}

namespace content {
class ResourceController;
struct ResourceResponse;

class CONTENT_EXPORT RedirectToBlobResourceHandler
    : public RedirectToFileResourceHandler {
 public:
  // Create a RedirectToBlobResourceHandler for |request| which wraps
  // |next_handler|.
  RedirectToBlobResourceHandler(
      std::unique_ptr<ResourceHandler> next_handler,
      base::WeakPtr<storage::BlobStorageContext> blob_storage_context,
      net::URLRequest* request);
  ~RedirectToBlobResourceHandler() override;

  // LayeredResourceHandler implementation:
  void OnResponseStarted(
      ResourceResponse* response,
      std::unique_ptr<ResourceController> controller) override;
  void OnDataDownloaded(int bytes_downloaded) override;

 protected:
  int OnResponseCompletedHook() override;
  void PopulateResponseData(ResourceResponse* response) override;

 private:
  void StoreFileStreamCallback(
      const CreateTemporaryFileStreamCallback& callback);

  void TemporaryFileCreated(base::File::Error,
                            std::unique_ptr<net::FileStream>,
                            storage::ShareableFileReference*);

  // Ensures that we have |amount| blob disk quota reserved. |directory_path|
  // and |new_blob_file_path| are optional.
  bool EnsureBlobStorageDiskQuotaReserved(int64_t amount,
                                          base::FilePath* directory_path,
                                          base::FilePath* new_blob_file_path);

  int64_t total_size_ = 0;
  base::WeakPtr<storage::BlobStorageContext> blob_storage_context_;

  bool saving_to_blob_dir_ = false;
  base::FilePath file_path_;
  scoped_refptr<ResourceResponse> response_pending_file_creation_;
  CreateTemporaryFileStreamCallback requested_file_stream_callback_;

  // Reserves disk space while we wait for the file to be downloaded. Used if
  // the resource response has a content length.
  std::unique_ptr<storage::BlobMemoryController::TempDiskAllocation>
      temp_disk_allocation_;
  std::unique_ptr<storage::BlobDataHandle> future_blob_handle_;

  base::WeakPtrFactory<RedirectToBlobResourceHandler> ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(RedirectToBlobResourceHandler);
};

}  // namespace content

#endif  // CONTENT_BROWSER_BLOB_STORAGE_REDIRECT_TO_BLOB_RESOURCE_HANDLER_H_
