// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/services/chrome_file_util/public/cpp/zip_file_creator.h"

#include <utility>

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/task_scheduler/post_task.h"
#include "chrome/services/chrome_file_util/public/interfaces/constants.mojom.h"
#include "components/filesystem/directory_impl.h"
#include "components/filesystem/lock_table.h"
#include "content/public/browser/browser_thread.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/service_manager/public/cpp/connector.h"

namespace {

// Creates the destination zip file only if it does not already exist.
base::File OpenFileHandleAsync(const base::FilePath& zip_path) {
  return base::File(zip_path, base::File::FLAG_CREATE | base::File::FLAG_WRITE);
}

}  // namespace

namespace chrome {

class ZipFileCreator::BindDirectoryParam
    : public base::RefCountedThreadSafe<BindDirectoryParam> {
 public:
  filesystem::mojom::DirectoryPtr directory_ptr_;

 private:
  friend class base::RefCountedThreadSafe<BindDirectoryParam>;

  ~BindDirectoryParam() = default;
};

ZipFileCreator::ZipFileCreator(
    const ResultCallback& callback,
    const base::FilePath& src_dir,
    const std::vector<base::FilePath>& src_relative_paths,
    const base::FilePath& dest_file)
    : callback_(callback),
      src_dir_(src_dir),
      src_relative_paths_(src_relative_paths),
      dest_file_(dest_file) {
  DCHECK(!callback_.is_null());
}

void ZipFileCreator::Start(service_manager::Connector* connector) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  base::PostTaskWithTraitsAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::Bind(&OpenFileHandleAsync, dest_file_),
      base::Bind(&ZipFileCreator::CreateZipFile, this,
                 base::Unretained(connector)));
}

ZipFileCreator::~ZipFileCreator() = default;

void ZipFileCreator::CreateZipFile(service_manager::Connector* connector,
                                   base::File file) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(!zip_file_creator_ptr_);

  if (!file.IsValid()) {
    LOG(ERROR) << "Failed to create dest zip file " << dest_file_.value();
    ReportDone(false);
    return;
  }

  scoped_refptr<BindDirectoryParam> param = new BindDirectoryParam();

  if (!directory_task_runner_) {
    directory_task_runner_ = base::CreateSequencedTaskRunnerWithTraits(
        {base::MayBlock(), base::TaskPriority::BACKGROUND,
         base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN});
  }

  directory_task_runner_->PostTaskAndReply(
      FROM_HERE, base::Bind(&ZipFileCreator::BindDirectory, this, param),
      base::Bind(&ZipFileCreator::OnDirectoryBound, this, connector,
                 base::Passed(&file), param));
}

void ZipFileCreator::BindDirectory(scoped_refptr<BindDirectoryParam> param) {
  auto directory_impl = std::make_unique<filesystem::DirectoryImpl>(
      src_dir_, scoped_refptr<filesystem::SharedTempDir>(),
      scoped_refptr<filesystem::LockTable>());
  filesystem::mojom::DirectoryPtr directory_ptr;
  mojo::MakeStrongBinding(std::move(directory_impl),
                          mojo::MakeRequest(&directory_ptr));
  param->directory_ptr_ = std::move(directory_ptr);
}

void ZipFileCreator::OnDirectoryBound(service_manager::Connector* connector,
                                      base::File file,
                                      scoped_refptr<BindDirectoryParam> param) {
  connector->BindInterface(chrome::mojom::kChromeFileUtilServiceName,
                           mojo::MakeRequest(&zip_file_creator_ptr_));
  zip_file_creator_ptr_.set_connection_error_handler(
      base::Bind(&ZipFileCreator::ReportDone, this, false));
  zip_file_creator_ptr_->CreateZipFile(
      std::move(param->directory_ptr_), src_dir_, src_relative_paths_,
      std::move(file), base::Bind(&ZipFileCreator::ReportDone, this));
}

void ZipFileCreator::ReportDone(bool success) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  zip_file_creator_ptr_.reset();
  base::ResetAndReturn(&callback_).Run(success);
}

}  // namespace chrome
