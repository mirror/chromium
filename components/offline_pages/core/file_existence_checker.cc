// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/file_existence_checker.h"

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/sequenced_task_runner.h"

namespace offline_pages {

namespace {

std::vector<base::FilePath> CheckForMissingFilesBlocking(
    std::vector<base::FilePath> file_paths) {
  std::vector<base::FilePath> missing_files;
  for (auto file_path : file_paths) {
    if (!base::PathExists(file_path))
      missing_files.push_back(file_path);
  }
  return missing_files;
}

}  // namespace

FileExistenceChecker::FileExistenceChecker(
    const scoped_refptr<base::SequencedTaskRunner>& blocking_task_runner)
    : blocking_task_runner_(blocking_task_runner) {}

FileExistenceChecker::~FileExistenceChecker() {}

void FileExistenceChecker::CheckForMissingFiles(
    std::vector<base::FilePath> file_paths,
    ResultCallback callback) {
  base::PostTaskAndReplyWithResult(
      blocking_task_runner_.get(), FROM_HERE,
      base::BindOnce(&CheckForMissingFilesBlocking, std::move(file_paths)),
      std::move(callback));
}

}  // namespace offline_pages
