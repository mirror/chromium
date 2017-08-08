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

namespace base {
class FileProxy;
}

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
  void PopulateResponseData(ResourceResponse* response) override;

 private:
  // Returns if we have enough quota.
  bool CreateBlobFileStream(int64_t suggested_content_length);

  void BlobFileCreated(std::unique_ptr<base::FileProxy> file_proxy,
                       scoped_refptr<base::SequencedTaskRunner> task_runner,
                       base::File::Error error);

  void StoreFileStreamCallback(
      const CreateTemporaryFileStreamCallback& callback);

  void TemporaryFileCreated(base::File::Error,
                            std::unique_ptr<net::FileStream>,
                            storage::ShareableFileReference*);

  int64_t total_size_ = 0;
  int64_t expected_size_ = 0;
  scoped_refptr<ChromeBlobStorageContext> blob_storage_context_;

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
