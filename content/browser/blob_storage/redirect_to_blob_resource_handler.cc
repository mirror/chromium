// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/blob_storage/redirect_to_blob_resource_handler.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback.h"
#include "base/callback_helpers.h"
#include "base/files/file_proxy.h"
#include "base/files/file_util.h"
#include "base/guid.h"
#include "base/task_scheduler/post_task.h"
#include "base/time/time.h"
#include "content/browser/blob_storage/chrome_blob_storage_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/resource_response.h"
#include "net/base/file_stream.h"
#include "net/base/net_errors.h"
#include "storage/browser/blob/blob_data_builder.h"
#include "storage/browser/blob/blob_data_handle.h"
#include "storage/browser/blob/blob_memory_controller.h"
#include "storage/browser/blob/blob_storage_context.h"
#include "storage/browser/blob/shareable_file_reference.h"
#include "storage/common/blob_storage/blob_storage_constants.h"

using storage::ShareableFileReference;
using base::File;

namespace content {

RedirectToBlobResourceHandler::RedirectToBlobResourceHandler(
    std::unique_ptr<ResourceHandler> next_handler,
    scoped_refptr<ChromeBlobStorageContext> blob_storage_context,
    net::URLRequest* request)
    : RedirectToFileResourceHandler(
          std::move(next_handler),
          base::Bind(&RedirectToBlobResourceHandler::StoreFileStreamCallback,
                     base::Unretained(this)),
          request),
      blob_storage_context_(blob_storage_context),
      ptr_factory_(this) {}

RedirectToBlobResourceHandler::~RedirectToBlobResourceHandler() = default;

void RedirectToBlobResourceHandler::OnResponseStarted(
    ResourceResponse* response,
    std::unique_ptr<ResourceController> controller) {
  if (saving_to_blob_dir_ && file_path_.empty()) {
    bool quota_success = CreateBlobFileStream(
        static_cast<int64_t>(response->head.content_length));
    if (!quota_success) {
      HoldController(std::move(controller));
      CancelWithError(net::ERR_FILE_NO_SPACE);
      return;
    }
  }

  RedirectToFileResourceHandler::OnResponseStarted(response,
                                                   std::move(controller));
}

void RedirectToBlobResourceHandler::OnDataDownloaded(int bytes_downloaded) {
  total_size_ += bytes_downloaded;
  RedirectToFileResourceHandler::OnDataDownloaded(bytes_downloaded);
}

int RedirectToBlobResourceHandler::OnResponseCompletedHook() {
  temp_disk_allocation_.reset();

  if (!future_blob_handle_ || file_path_.empty())
    return net::OK;

  if (future_blob_handle_->IsBroken())
    return net::ERR_FILE_NO_SPACE;

  storage::BlobStorageContext* context = blob_storage_context_->context();
  DCHECK(context);
  if (total_size_ > expected_size_ &&
      !context->memory_controller().CanReserveDiskQuota(total_size_)) {
    context->CancelBuildingBlob(future_blob_handle_->uuid(),
                                storage::BlobStatus::ERR_OUT_OF_MEMORY);
    return net::ERR_FILE_NO_SPACE;
  }
  storage::BlobDataBuilder builder(future_blob_handle_->uuid());
  builder.AppendFile(file_path_, 0ull, total_size_, base::Time());
  context->BuildPreregisteredBlob(
      builder, storage::BlobStorageContext::TransportAllowedCallback());
  return net::OK;
}

void RedirectToBlobResourceHandler::PopulateResponseData(
    ResourceResponse* response) {
  storage::BlobStorageContext* context = blob_storage_context_->context();
  future_blob_handle_ = context->AddFutureBlob(
      base::GenerateGUID(), "net-internals/transport-blob-handle", "");
  response->head.blob_uuid = future_blob_handle_->uuid();
}

bool RedirectToBlobResourceHandler::CreateBlobFileStream(
    int64_t suggested_content_length) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  storage::BlobStorageContext* context = blob_storage_context_->context();

  if (suggested_content_length > 0)
    expected_size_ = static_cast<uint64_t>(suggested_content_length);
  else
    expected_size_ = 0;

  base::FilePath directory_path;
  temp_disk_allocation_ =
      context->mutable_memory_controller()->ReserveTemporaryDiskQuota(
          expected_size_, &directory_path, &file_path_);
  if (!temp_disk_allocation_) {
    return false;
  }

  scoped_refptr<base::SequencedTaskRunner> task_runner =
      base::CreateSequencedTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
           base::TaskShutdownBehavior::BLOCK_SHUTDOWN});

  task_runner->PostTask(
      FROM_HERE,
      base::Bind(base::IgnoreResult(&base::CreateDirectory), directory_path));
  std::unique_ptr<base::FileProxy> file_proxy(
      new base::FileProxy(task_runner.get()));
  base::FileProxy* proxy = file_proxy.get();
  proxy->CreateOrOpen(
      file_path_,
      File::FLAG_WRITE | File::FLAG_ASYNC | File::FLAG_CREATE_ALWAYS,
      base::Bind(&RedirectToBlobResourceHandler::BlobFileCreated,
                 ptr_factory_.GetWeakPtr(), base::Passed(&file_proxy),
                 base::Passed(&task_runner)));

  return true;
}

void RedirectToBlobResourceHandler::BlobFileCreated(
    std::unique_ptr<base::FileProxy> file_proxy,
    scoped_refptr<base::SequencedTaskRunner> task_runner,
    base::File::Error error) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  if (!file_proxy->IsValid()) {
    base::ResetAndReturn(&requested_file_stream_callback_)
        .Run(error, std::unique_ptr<net::FileStream>(), NULL);
    return;
  }

  // Cancelled or not, create the deletable_file so the temporary is cleaned up.
  scoped_refptr<ShareableFileReference> deletable_file =
      ShareableFileReference::GetOrCreate(
          file_path_, ShareableFileReference::DELETE_ON_FINAL_RELEASE,
          task_runner.get());

  std::unique_ptr<net::FileStream> file_stream(
      new net::FileStream(file_proxy->TakeFile(), task_runner));
  base::ResetAndReturn(&requested_file_stream_callback_)
      .Run(error, std::move(file_stream), deletable_file.get());
}

void RedirectToBlobResourceHandler::StoreFileStreamCallback(
    const CreateTemporaryFileStreamCallback& callback) {
  storage::BlobStorageContext* context = blob_storage_context_->context();
  saving_to_blob_dir_ =
      context && context->memory_controller().file_paging_enabled();
  if (saving_to_blob_dir_) {
    requested_file_stream_callback_ = callback;
  } else {
    CreateTemporaryFileStream(
        base::Bind(&RedirectToBlobResourceHandler::TemporaryFileCreated,
                   ptr_factory_.GetWeakPtr()));
  }
}

void RedirectToBlobResourceHandler::TemporaryFileCreated(
    base::File::Error error,
    std::unique_ptr<net::FileStream> stream,
    storage::ShareableFileReference* reference) {
  if (error != base::File::FILE_OK) {
    blob_storage_context_->context()->CancelBuildingBlob(
        future_blob_handle_->uuid(), storage::BlobStatus::ERR_OUT_OF_MEMORY);
  } else {
    file_path_ = reference->path();
  }
  base::ResetAndReturn(&requested_file_stream_callback_)
      .Run(error, std::move(stream), reference);
}

}  // namespace content
