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
#include "content/browser/loader/redirect_to_file_resource_handler.h"
#include "content/browser/loader/temporary_file_stream.h"
#include "content/common/content_export.h"
#include "storage/browser/blob/blob_data_handle.h"
#include "storage/browser/blob/blob_memory_controller.h"

namespace content {
class ChromeBlobStorageContext;
class ResourceController;
struct ResourceResponse;

class CONTENT_EXPORT RedirectToBlobResourceHandler
    : public RedirectToFileResourceHandler {
 public:
  // Create a RedirectToBlobResourceHandler for |request| which wraps
  // |next_handler|.
  RedirectToBlobResourceHandler(
      std::unique_ptr<ResourceHandler> next_handler,
      scoped_refptr<ChromeBlobStorageContext> blob_storage_context,
      net::URLRequest* request);
  ~RedirectToBlobResourceHandler() override;

  // LayeredResourceHandler implementation:
  void OnResponseStarted(
      ResourceResponse* response,
      std::unique_ptr<ResourceController> controller) override;
  void OnDataDownloaded(int bytes_downloaded) override;

 protected:
  int OnResponseCompletedHook() override;

 private:
  static void CreateBlobFileStream(const base::FilePath& dir,
                                   const base::FilePath& file,
                                   CreateTemporaryFileStreamCallback callback);

  // If we don't have the file path then we store the callback for alter.
  void MaybeCreateBlobFile(const CreateTemporaryFileStreamCallback& callback);

  void TemporaryFileCreated(CreateTemporaryFileStreamCallback callback,
                            base::File::Error,
                            std::unique_ptr<net::FileStream>,
                            storage::ShareableFileReference*);

  uint64_t total_size_ = 0;
  uint64_t expected_size_ = 0;
  scoped_refptr<ChromeBlobStorageContext> blob_storage_context_;

  base::FilePath directory_path_;
  base::FilePath file_path_;
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
