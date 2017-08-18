// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/blob_storage/redirect_to_blob_resource_handler.h"

#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback.h"
#include "base/callback_helpers.h"
#include "base/files/file_proxy.h"
#include "base/files/file_util.h"
#include "base/guid.h"
#include "base/task_runner_util.h"
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

namespace content {
namespace {
using storage::ShareableFileReference;
using base::File;
using base::FilePath;

std::pair<File, File::Error> CreateBlobFileOnFileRunner(
    FilePath directory_path,
    FilePath blob_file_path) {
  File::Error error = File::FILE_OK;
  if (!base::CreateDirectoryAndGetError(directory_path, &error))
    return std::make_pair(File(), error);
  File file;
  file.Initialize(blob_file_path, File::FLAG_WRITE | File::FLAG_ASYNC |
                                      File::FLAG_CREATE_ALWAYS);
  return std::make_pair(std::move(file), file.error_details());
}

void DidCreateBlobFile(CreateTemporaryFileStreamCallback callback,
                       scoped_refptr<base::SequencedTaskRunner> task_runner,
                       base::FilePath file_path,
                       std::pair<File, File::Error> creation_result) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  File& file = creation_result.first;
  File::Error error_code = creation_result.second;
  if (!file.IsValid()) {
    callback.Run(error_code, std::unique_ptr<net::FileStream>(), nullptr);
    return;
  }
  // Cancelled or not, create the deletable_file so the temporary is cleaned up.
  scoped_refptr<ShareableFileReference> deletable_file =
      ShareableFileReference::GetOrCreate(
          file_path, ShareableFileReference::DELETE_ON_FINAL_RELEASE,
          task_runner.get());

  std::unique_ptr<net::FileStream> file_stream(
      new net::FileStream(std::move(file), std::move(task_runner)));
  callback.Run(error_code, std::move(file_stream), deletable_file.get());
}

void CreateBlobFileStream(CreateTemporaryFileStreamCallback callback,
                          FilePath directory_path,
                          FilePath blob_file_path) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  scoped_refptr<base::SequencedTaskRunner> task_runner =
      base::CreateSequencedTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
           base::TaskShutdownBehavior::BLOCK_SHUTDOWN});

  PostTaskAndReplyWithResult(
      task_runner.get(), FROM_HERE,
      base::BindOnce(&CreateBlobFileOnFileRunner, base::Passed(&directory_path),
                     blob_file_path),
      base::BindOnce(&DidCreateBlobFile, base::Passed(&callback), task_runner,
                     blob_file_path));
}

}  // namespace

RedirectToBlobResourceHandler::RedirectToBlobResourceHandler(
    std::unique_ptr<ResourceHandler> next_handler,
    base::WeakPtr<storage::BlobStorageContext> blob_storage_context,
    net::URLRequest* request)
    : RedirectToFileResourceHandler(
          std::move(next_handler),
          base::Bind(&RedirectToBlobResourceHandler::StoreFileStreamCallback,
                     base::Unretained(this)),
          request),
      blob_storage_context_(std::move(blob_storage_context)),
      ptr_factory_(this) {}

RedirectToBlobResourceHandler::~RedirectToBlobResourceHandler() = default;

void RedirectToBlobResourceHandler::OnResponseStarted(
    ResourceResponse* response,
    std::unique_ptr<ResourceController> controller) {
  if (saving_to_blob_dir_ && !blob_storage_context_) {
    HoldController(std::move(controller));
    CancelWithError(net::ERR_FILE_NO_SPACE);
    return;
  }

  if (!saving_to_blob_dir_ || !file_path_.empty()) {
    RedirectToFileResourceHandler::OnResponseStarted(response,
                                                     std::move(controller));
    return;
  }

  int64_t expected_size;
  if (response->head.content_length > 0)
    expected_size = response->head.content_length;
  else
    expected_size = 0;

  FilePath directory_path;
  if (!EnsureBlobStorageDiskQuotaReserved(expected_size, &directory_path,
                                          &file_path_)) {
    HoldController(std::move(controller));
    CancelWithError(net::ERR_FILE_NO_SPACE);
    return;
  }

  CreateBlobFileStream(base::ResetAndReturn(&requested_file_stream_callback_),
                       std::move(directory_path), file_path_);

  RedirectToFileResourceHandler::OnResponseStarted(response,
                                                   std::move(controller));
}

void RedirectToBlobResourceHandler::OnDataDownloaded(int bytes_downloaded) {
  total_size_ += bytes_downloaded;
  DCHECK_GE(total_size_, 0);
  if (!EnsureBlobStorageDiskQuotaReserved(total_size_, nullptr, nullptr)) {
    if (has_controller()) {
      CancelWithError(net::ERR_FILE_NO_SPACE);
    } else {
      OutOfBandCancel(net::ERR_FILE_NO_SPACE, true /* tell_renderer */);
    }
    return;
  }
  RedirectToFileResourceHandler::OnDataDownloaded(bytes_downloaded);
}

int RedirectToBlobResourceHandler::OnResponseCompletedHook() {
  temp_disk_allocation_.reset();

  if (!future_blob_handle_ || file_path_.empty())
    return net::OK;

  if (future_blob_handle_->IsBroken())
    return net::ERR_FILE_NO_SPACE;

  if (!blob_storage_context_)
    return net::ERR_FAILED;

  DCHECK(!temp_disk_allocation_ || temp_disk_allocation_->quota_reserved() >=
                                       static_cast<uint64_t>(total_size_));
  storage::BlobDataBuilder builder(future_blob_handle_->uuid());
  builder.AppendFile(file_path_, 0ull, total_size_, base::Time());
  blob_storage_context_->BuildPreregisteredBlob(
      builder, storage::BlobStorageContext::TransportAllowedCallback());
  return net::OK;
}

void RedirectToBlobResourceHandler::PopulateResponseData(
    ResourceResponse* response) {
  DCHECK(blob_storage_context_);
  future_blob_handle_ = blob_storage_context_->AddFutureBlob(
      base::GenerateGUID(), "net-internals/transport-blob-handle", "");
  response->head.blob_uuid = future_blob_handle_->uuid();
}

void RedirectToBlobResourceHandler::StoreFileStreamCallback(
    const CreateTemporaryFileStreamCallback& callback) {
  saving_to_blob_dir_ =
      !!blob_storage_context_ &&
      blob_storage_context_->memory_controller().file_paging_enabled();
  requested_file_stream_callback_ = callback;
  if (!saving_to_blob_dir_) {
    CreateTemporaryFileStream(
        base::Bind(&RedirectToBlobResourceHandler::TemporaryFileCreated,
                   ptr_factory_.GetWeakPtr()));
  }
}

void RedirectToBlobResourceHandler::TemporaryFileCreated(
    File::Error error,
    std::unique_ptr<net::FileStream> stream,
    storage::ShareableFileReference* reference) {
  if (error == File::FILE_OK) {
    file_path_ = reference->path();
  } else if (blob_storage_context_ && future_blob_handle_) {
    blob_storage_context_->CancelBuildingBlob(
        future_blob_handle_->uuid(), storage::BlobStatus::ERR_OUT_OF_MEMORY);
  }
  base::ResetAndReturn(&requested_file_stream_callback_)
      .Run(error, std::move(stream), reference);
}

bool RedirectToBlobResourceHandler::EnsureBlobStorageDiskQuotaReserved(
    int64_t amount,
    FilePath* directory_path,
    FilePath* new_blob_file_path) {
  DCHECK_LE(0, amount);
  if (!blob_storage_context_)
    return false;
  temp_disk_allocation_.reset();

  // Note: We must ask for quota even if amount is 0, as we may need the
  // |directory_path| & |new_blob_file_path| populated.
  uint64_t unsigned_amount = static_cast<uint64_t>(amount);
  if (!temp_disk_allocation_ ||
      unsigned_amount > temp_disk_allocation_->quota_reserved()) {
    temp_disk_allocation_ =
        blob_storage_context_->mutable_memory_controller()
            ->ReserveTemporaryDiskQuota(unsigned_amount, directory_path,
                                        new_blob_file_path);
    return !!temp_disk_allocation_;
  }
  return true;
}

}  // namespace content
