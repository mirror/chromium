// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/image_writer_private/operation.h"

#include <utility>

#include "base/files/file_enumerator.h"
#include "base/files/file_util.h"
#include "base/task_scheduler/post_task.h"
#include "build/build_config.h"
#include "chrome/browser/extensions/api/image_writer_private/error_messages.h"
#include "chrome/browser/extensions/api/image_writer_private/operation_manager.h"
#include "chrome/browser/extensions/api/image_writer_private/unzip_helper.h"
#include "content/public/browser/browser_thread.h"

namespace extensions {
namespace image_writer {

using content::BrowserThread;

namespace {

const int kMD5BufferSize = 1024;

}  // namespace

constexpr base::TaskTraits Operation::kBlockingTaskTraits;

Operation::Operation(base::WeakPtr<OperationManager> manager,
                     const ExtensionId& extension_id,
                     const std::string& device_path,
                     const base::FilePath& download_folder)
    : manager_(manager),
      extension_id_(extension_id),
#if defined(OS_WIN)
      device_path_(base::FilePath::FromUTF8Unsafe(device_path)),
#else
      device_path_(device_path),
#endif
      stage_(image_writer_api::STAGE_UNKNOWN),
      progress_(0),
      download_folder_(download_folder),
      task_runner_(
          base::CreateSequencedTaskRunnerWithTraits(kBlockingTaskTraits)) {
  printf("+Operation\n");
}

Operation::~Operation() {}

void Operation::Cancel() {
  DCHECK(IsRunningInCorrectSequence());

  stage_ = image_writer_api::STAGE_NONE;

  CleanUp();
}

void Operation::Abort() {
  DCHECK(IsRunningInCorrectSequence());
  Error(error::kAborted);
}

int Operation::GetProgress() {
  return progress_;
}

image_writer_api::Stage Operation::GetStage() {
  return stage_;
}

void Operation::PostTask(base::OnceClosure task) {
  task_runner_->PostTask(FROM_HERE, std::move(task));
}

void Operation::Start() {
  DCHECK(IsRunningInCorrectSequence());
#if defined(OS_CHROMEOS)
  if (download_folder_.empty() ||
      !temp_dir_.CreateUniqueTempDirUnderPath(download_folder_)) {
#else
  if (!temp_dir_.CreateUniqueTempDir()) {
#endif
    Error(error::kTempDirError);
    return;
  }

  AddCleanUpFunction(
      base::BindOnce(base::IgnoreResult(&base::ScopedTempDir::Delete),
                     base::Unretained(&temp_dir_)));

  StartImpl();
}

void Operation::OnUnzipOpenComplete(const base::FilePath& image_path) {
  image_path_ = image_path;
}

void Operation::Unzip(const base::Closure& continuation) {
  printf("Operation::Unzip\n");
  DCHECK(IsRunningInCorrectSequence());
  if (IsCancelled()) {
    return;
  }

  if (image_path_.Extension() != FILE_PATH_LITERAL(".zip")) {
    PostTask(std::move(continuation));
    return;
  }

  SetStage(image_writer_api::STAGE_UNZIP);

  //unzip_helper_ = make_scoped_refptr(new UnzipHelper(task_runner()));
  auto unzip_helper = make_scoped_refptr(new UnzipHelper(task_runner()));
  printf("Operation::Unzip\n");
  unzip_helper->Unzip(
      image_path_,
      temp_dir_.GetPath(),
      base::Bind(&Operation::OnUnzipOpenComplete, this),
      base::Bind(&Operation::CompleteAndContinue, this, continuation),
      base::Bind(&Operation::OnUnzipFailure, this),
      base::Bind(&Operation::OnUnzipProgress, this));
  printf("Operation::Unzip\n");
//  if (error) {
//    Error(error);
//    return;
//  }
//  // OnUnzipEntryInfo
//  // OnUnzipFailure
//  // OnUnzipProgress
//  // OnUnzipComplete
//  if (!(zip_reader_->Open(image_path_) && zip_reader_->AdvanceToNextEntry() &&
//        zip_reader_->OpenCurrentEntryInZip())) {
//    Error(error::kUnzipGenericError);
//    return;
//  }
//
//  if (zip_reader_->HasMore()) {
//    Error(error::kUnzipInvalidArchive);
//    return;
//  }
//
//  // Create a new target to unzip to.  The original file is opened by the
//  // zip_reader_.
//  zip::ZipReader::EntryInfo* entry_info = zip_reader_->current_entry_info();
//  if (entry_info) {
//    image_path_ =
//        temp_dir_.GetPath().Append(entry_info->file_path().BaseName());
//  } else {
//    Error(error::kTempDirError);
//    return;
//  }
//
//  // |zip_reader_| doesn't support OnceClosure, so save |continuation|.
//  //StoreCompletionClosure(std::move(continuation));
//
//  zip_reader_->ExtractCurrentEntryToFilePathAsync(
//      image_path_,
//      base::Bind(&Operation::CompleteAndContinue, this, continuation),
//      base::Bind(&Operation::OnUnzipFailure, this),
//      base::Bind(&Operation::OnUnzipProgress,
//                 this,
//                 zip_reader_->current_entry_info()->original_size()));
}

void Operation::Finish() {
  printf("Operation::Finish\n");
  DCHECK(IsRunningInCorrectSequence());

  CleanUp();

  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce(&OperationManager::OnComplete, manager_, extension_id_));
}

void Operation::Error(const std::string& error_message) {
  DCHECK(IsRunningInCorrectSequence());

  printf("Posting OnError\n");
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce(&OperationManager::OnError, manager_, extension_id_,
                     stage_, progress_, error_message));

  CleanUp();
}

void Operation::SetProgress(int progress) {
  DCHECK(IsRunningInCorrectSequence());

  if (progress <= progress_) {
    return;
  }

  if (IsCancelled()) {
    return;
  }

  progress_ = progress;

  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce(&OperationManager::OnProgress, manager_, extension_id_,
                     stage_, progress_));
}

void Operation::SetStage(image_writer_api::Stage stage) {
  DCHECK(IsRunningInCorrectSequence());

  if (IsCancelled())
    return;

  stage_ = stage;
  progress_ = 0;

  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce(&OperationManager::OnProgress, manager_, extension_id_,
                     stage_, progress_));
}

bool Operation::IsCancelled() {
  DCHECK(IsRunningInCorrectSequence());

  return stage_ == image_writer_api::STAGE_NONE;
}

void Operation::AddCleanUpFunction(base::OnceClosure callback) {
  DCHECK(IsRunningInCorrectSequence());
  cleanup_functions_.push_back(std::move(callback));
}

void Operation::CompleteAndContinue(const base::Closure& continuation) {
  DCHECK(IsRunningInCorrectSequence());
  SetProgress(kProgressComplete);
  PostTask(continuation);
}

//void Operation::StoreCompletionClosure(base::OnceClosure closure) {
//  completion_closure_ = std::move(closure);
//}

#if !defined(OS_CHROMEOS)
void Operation::StartUtilityClient() {
  DCHECK(IsRunningInCorrectSequence());
  if (!image_writer_client_.get()) {
    printf("Calling Create.\n");
    image_writer_client_ = ImageWriterUtilityClient::Create();
    AddCleanUpFunction(base::BindOnce(&Operation::StopUtilityClient, this));
  }
}

void Operation::StopUtilityClient() {
  DCHECK(IsRunningInCorrectSequence());
  image_writer_client_->Shutdown();
}

void Operation::WriteImageProgress(int64_t total_bytes, int64_t curr_bytes) {
  DCHECK(IsRunningInCorrectSequence());
  if (IsCancelled()) {
    return;
  }

  int progress = kProgressComplete * curr_bytes / total_bytes;

  if (progress > GetProgress()) {
    SetProgress(progress);
  }
}
#endif

void Operation::GetMD5SumOfFile(
    const base::FilePath& file_path,
    int64_t file_size,
    int progress_offset,
    int progress_scale,
    base::OnceCallback<void(const std::string&)> callback) {
  DCHECK(IsRunningInCorrectSequence());
  if (IsCancelled()) {
    return;
  }

  base::MD5Init(&md5_context_);

  base::File file(file_path, base::File::FLAG_OPEN | base::File::FLAG_READ);
  if (!file.IsValid()) {
    Error(error::kImageOpenError);
    return;
  }

  if (file_size <= 0) {
    file_size = file.GetLength();
    if (file_size < 0) {
      Error(error::kImageOpenError);
      return;
    }
  }

  // TODO(lazyboy): Remove PostTask.
  PostTask(
      base::BindOnce(&Operation::MD5Chunk, this, Passed(std::move(file)), 0,
                     file_size, progress_offset, progress_scale, std::move(callback)));
}

bool Operation::IsRunningInCorrectSequence() const {
  base::ThreadRestrictions::AssertIOAllowed();
  return task_runner_->RunsTasksInCurrentSequence();
}

void Operation::MD5Chunk(
    base::File file,
    int64_t bytes_processed,
    int64_t bytes_total,
    int progress_offset,
    int progress_scale,
    base::OnceCallback<void(const std::string&)> callback) {
  DCHECK(IsRunningInCorrectSequence());
  if (IsCancelled())
    return;

  CHECK_LE(bytes_processed, bytes_total);

  std::unique_ptr<char[]> buffer(new char[kMD5BufferSize]);
  int read_size = std::min(bytes_total - bytes_processed,
                           static_cast<int64_t>(kMD5BufferSize));

  if (read_size == 0) {
    // Nothing to read, we are done.
    base::MD5Digest digest;
    base::MD5Final(&digest, &md5_context_);
    std::move(callback).Run(base::MD5DigestToBase16(digest));
  } else {
    int len = file.Read(bytes_processed, buffer.get(), read_size);

    if (len == read_size) {
      // Process data.
      base::MD5Update(&md5_context_, base::StringPiece(buffer.get(), len));
      int percent_curr =
          ((bytes_processed + len) * progress_scale) / bytes_total +
          progress_offset;
      SetProgress(percent_curr);

      PostTask(
          base::BindOnce(&Operation::MD5Chunk, this, Passed(std::move(file)),
                         bytes_processed + len, bytes_total, progress_offset,
                         progress_scale, std::move(callback)));
      // Skip closing the file.
      return;
    } else {
      // We didn't read the bytes we expected.
      Error(error::kHashReadError);
    }
  }
}

void Operation::OnUnzipFailure(const std::string& error) {
  DCHECK(IsRunningInCorrectSequence());
  Error(error);
}

void Operation::OnUnzipProgress(int64_t total_bytes, int64_t progress_bytes) {
  DCHECK(IsRunningInCorrectSequence());

  int progress_percent = kProgressComplete * progress_bytes / total_bytes;
  SetProgress(progress_percent);
}

//void Operation::OnUnzipComplete() {
//}
//void Operation::OnUnzipEntryInfo(zip::ZipReader* entry_info) {
//}

void Operation::CleanUp() {
  printf("Operation::CleanUp\n");
  DCHECK(IsRunningInCorrectSequence());
  for (base::OnceClosure& cleanup_function : cleanup_functions_)
    std::move(cleanup_function).Run();
  cleanup_functions_.clear();
}

// Operation::ZipReaderHelper --------------------------------------------------

/*
class Operation::ZipReaderHelper {
 public:
  ZipReaderHelper(
      Operation* operation,
      scoped_refptr<SequencedTaskRunner> owner_task_runner)
      : operation_(operation), owner_task_runner_(owner_task_runner) {
  }
  ~ZipReaderHelper() {}
 private:

  Operation* const operation_;  // Owns us.
  DISALLOW_COPY_AND_ASSIGN(ZipReaderHelper);
};
*/

}  // namespace image_writer
}  // namespace extensions
