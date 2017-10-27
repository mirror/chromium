// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/notifications/temp_file_keeper.h"

#include "base/files/file_enumerator.h"
#include "base/files/file_util.h"
#include "base/hash.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "ui/gfx/image/image.h"
#include "url/gurl.h"

using base::FilePath;

namespace {

// How long to keep the temp files before deleting them.
constexpr base::TimeDelta kDeletionDelay = base::TimeDelta::FromSeconds(7);

// Writes |data| to a new temporary file and returns the ResourceFile
// that holds it.
FilePath WriteDataToTmpFile(const FilePath& file_path,
                            const base::string16& prefix,
                            const scoped_refptr<base::RefCountedMemory>& data) {
  int data_len = data->size();
  if (data_len == 0)
    return FilePath();

  FilePath temp_file;
  if (!base::CreateTemporaryFileWithPrefixInDir(file_path, prefix, &temp_file))
    return FilePath();
  if (base::WriteFile(temp_file, data->front_as<char>(), data_len) != data_len)
    return FilePath();
  return temp_file;
}

}  // namespace

TempFileKeeperImpl::TempFileKeeperImpl(const base::string16& dir_prefix,
                                       base::SequencedTaskRunner* task_runner)
    : dir_prefix_(dir_prefix),
      cleanup_performed_(false),
      task_runner_(task_runner) {}

TempFileKeeperImpl::~TempFileKeeperImpl() {}

void DelFile(const FilePath& file, bool test) {}

FilePath TempFileKeeperImpl::RegisterTemporaryImage(
    gfx::Image image,
    const GURL& origin,
    const base::string16& prefix) {
  ResetTempFiles();

  base::string16 file_prefix = prefix +
                               base::UintToString16(base::Hash(origin.spec())) +
                               base::UTF8ToUTF16("_");
  FilePath temp_file =
      WriteDataToTmpFile(temp_path_, file_prefix, image.As1xPNGBytes());

  // Add a future task to try to delete the file. It is OK to fail, the file
  // will get picked up later.
  task_runner_->PostDelayedTask(
      FROM_HERE,
      base::Bind(base::IgnoreResult(&base::DeleteFile), temp_file,
                 /*recursive=*/true),
      kDeletionDelay);

  return temp_file;
}

void TempFileKeeperImpl::ResetTempFiles() {
  if (cleanup_performed_)
    return;

  DeleteOrphanedImages();

  // TODO(finnur): Create this in the data dir, not in the temp directory.
  base::FilePath::StringType prefix(dir_prefix_);
  base::CreateNewTempDirectory(prefix, &temp_path_);

  cleanup_performed_ = true;
}

void TempFileKeeperImpl::DeleteOrphanedImages() {
  FilePath temp;
  if (!base::GetTempDir(&temp)) {
    return;
  }

  // Find all temp folders that previous runs created and delete them.
  base::FilePath::StringType search_prefix(dir_prefix_ + L"*");
  base::FileEnumerator enumerator(
      temp, false, base::FileEnumerator::DIRECTORIES, search_prefix,
      base::FileEnumerator::FolderSearchPolicy::MATCH_ONLY);
  for (auto file = enumerator.Next(); !file.empty(); file = enumerator.Next()) {
    DeleteFile(file, /*recursive=*/true);
  }
}
