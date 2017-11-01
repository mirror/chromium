// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/notifications/notification_image_retainer.h"

#include "base/files/file_enumerator.h"
#include "base/files/file_util.h"
#include "base/hash.h"
#include "base/path_service.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "chrome/common/chrome_paths.h"
#include "ui/gfx/image/image.h"
#include "url/gurl.h"

using base::FilePath;

namespace {

// How long to keep the temp files before deleting them.
constexpr base::TimeDelta kDeletionDelay = base::TimeDelta::FromSeconds(7);

// Writes |data| to a new temporary file and returns the ResourceFile
// that holds it.
FilePath WriteDataToTmpFile(const FilePath& file_path,
                            const base::string16& subdirectory,
                            const scoped_refptr<base::RefCountedMemory>& data) {
  int data_len = data->size();
  if (data_len == 0)
    return FilePath();

  FilePath new_temp = file_path.Append(subdirectory);
  if (!base::CreateDirectoryAndGetError(new_temp, nullptr))
    return FilePath();

  FilePath temp_file;
  if (!base::CreateTemporaryFileInDir(new_temp, &temp_file))
    return FilePath();
  if (base::WriteFile(temp_file, data->front_as<char>(), data_len) != data_len)
    return FilePath();
  return temp_file;
}

}  // namespace

NotificationImageRetainerImpl::NotificationImageRetainerImpl(
    const base::string16& dir_prefix,
    base::SequencedTaskRunner* task_runner)
    : dir_prefix_(dir_prefix),
      cleanup_performed_(false),
      task_runner_(task_runner) {}

NotificationImageRetainerImpl::~NotificationImageRetainerImpl() {}

FilePath NotificationImageRetainerImpl::RegisterTemporaryImage(
    gfx::Image image,
    const std::string& profile_id,
    const GURL& origin) {
  ResetTempFiles();

  base::string16 directory = base::UTF8ToUTF16(profile_id) + L"_" +
                             base::UintToString16(base::Hash(origin.spec()));
  FilePath temp_file =
      WriteDataToTmpFile(temp_path_, directory, image.As1xPNGBytes());

  // Add a future task to try to delete the file. It is OK to fail, the file
  // will get picked up later.
  task_runner_->PostDelayedTask(
      FROM_HERE,
      base::Bind(base::IgnoreResult(&base::DeleteFile), temp_file,
                 /*recursive=*/true),
      kDeletionDelay);

  return temp_file;
}

void NotificationImageRetainerImpl::ResetTempFiles() {
  if (cleanup_performed_)
    return;

  DeleteOrphanedImages();

  bool success = false;
  base::FilePath::StringType prefix(dir_prefix_);

  FilePath data_dir = GetBaseTempDirectory().Append(dir_prefix_);
  if (base::CreateDirectoryAndGetError(data_dir, nullptr)) {
    temp_path_ = data_dir;
    success = true;
  }

  // Last resort, fall back to temp directory.
  if (!success)
    base::CreateNewTempDirectory(prefix, &temp_path_);

  cleanup_performed_ = true;
}

void NotificationImageRetainerImpl::DeleteOrphanedImages() {
  FilePath temp = GetBaseTempDirectory();
  if (temp.empty())
    return;

  // Find all temp folders that previous runs created and delete them.
  base::FilePath::StringType search_prefix(dir_prefix_);
  base::FileEnumerator enumerator(
      temp, false, base::FileEnumerator::DIRECTORIES, search_prefix,
      base::FileEnumerator::FolderSearchPolicy::MATCH_ONLY);
  for (auto file = enumerator.Next(); !file.empty(); file = enumerator.Next()) {
    DeleteFile(file, /*recursive=*/true);
  }
}

FilePath NotificationImageRetainerImpl::GetBaseTempDirectory() {
  FilePath data_dir;
  if (base::PathService::Get(chrome::DIR_USER_DATA, &data_dir))
    return data_dir;
  return base::FilePath();
}
