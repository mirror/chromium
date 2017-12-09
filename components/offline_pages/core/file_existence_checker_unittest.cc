// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/file_existence_checker.h"

#include <vector>

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/sequenced_task_runner.h"
#include "base/test/test_simple_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace offline_pages {
namespace {

std::set<base::FilePath> CheckForMissingFiles(
    FileExistenceChecker* checker,
    base::TestSimpleTaskRunner* task_runner,
    std::vector<base::FilePath> file_paths) {
  std::set<base::FilePath> missing_files;
  checker->CheckForMissingFiles(
      std::move(file_paths), base::BindOnce(
                                 [](std::set<base::FilePath>* alias,
                                    std::vector<base::FilePath> result) {
                                   alias->insert(result.begin(), result.end());
                                 },
                                 &missing_files));
  task_runner->RunUntilIdle();
  return missing_files;
}

}  // namespace

class FileExistenceCheckerTest : public testing::Test {
 public:
  FileExistenceCheckerTest();
  ~FileExistenceCheckerTest() override;

  std::vector<base::FilePath> CreateTestFiles(size_t count);

  base::TestSimpleTaskRunner* task_runner() { return task_runner_.get(); }

 private:
  base::ScopedTempDir temp_directory_;
  scoped_refptr<base::TestSimpleTaskRunner> task_runner_;
  base::ThreadTaskRunnerHandle task_runner_handle_;
};

FileExistenceCheckerTest::FileExistenceCheckerTest()
    : task_runner_(new base::TestSimpleTaskRunner),
      task_runner_handle_(task_runner_) {
  EXPECT_TRUE(temp_directory_.CreateUniqueTempDir());
}

FileExistenceCheckerTest::~FileExistenceCheckerTest() {}

std::vector<base::FilePath> FileExistenceCheckerTest::CreateTestFiles(
    size_t count) {
  base::FilePath file_path;
  std::vector<base::FilePath> created_files;
  for (size_t i = 0; i < count; i++) {
    EXPECT_TRUE(
        base::CreateTemporaryFileInDir(temp_directory_.GetPath(), &file_path));
    created_files.push_back(file_path);
  }

  return created_files;
}

TEST_F(FileExistenceCheckerTest, NoFilesMissing) {
  std::vector<base::FilePath> files = CreateTestFiles(2);
  FileExistenceChecker checker(task_runner());
  std::set<base::FilePath> missing_files =
      CheckForMissingFiles(&checker, task_runner(), files);
  EXPECT_TRUE(missing_files.empty());
}

TEST_F(FileExistenceCheckerTest, MissingFileFound) {
  std::vector<base::FilePath> files = CreateTestFiles(2);

  base::DeleteFile(files[0], false /* recursive */);

  FileExistenceChecker checker(task_runner());
  std::set<base::FilePath> missing_files =
      CheckForMissingFiles(&checker, task_runner(), files);

  EXPECT_EQ(1UL, missing_files.size());
  EXPECT_EQ(1UL, missing_files.count(files[0]));
  EXPECT_EQ(0UL, missing_files.count(files[1]));
}

}  // namespace offline_pages
