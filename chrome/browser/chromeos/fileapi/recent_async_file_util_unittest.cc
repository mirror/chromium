// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>
#include <utility>
#include <vector>

#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/files/scoped_temp_dir.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/memory/ref_counted.h"
#include "chrome/browser/chromeos/fileapi/recent_async_file_util.h"
#include "chrome/browser/chromeos/fileapi/recent_file.h"
#include "chrome/browser/chromeos/fileapi/recent_model.h"
#include "chrome/browser/chromeos/fileapi/recent_util.h"
#include "chrome/browser/chromeos/fileapi/test/fake_recent_source.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "storage/browser/fileapi/file_system_context.h"
#include "storage/browser/fileapi/file_system_operation.h"
#include "storage/browser/fileapi/file_system_operation_context.h"
#include "storage/browser/test/test_file_system_context.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace chromeos {
namespace {

class RecentAsyncFileUtilTest : public testing::Test {
 public:
  RecentAsyncFileUtilTest()
      : testing_profile_manager_(TestingBrowserProcess::GetGlobal()) {}

 protected:
  void SetUp() override {
    ASSERT_TRUE(testing_profile_manager_.SetUp());
    profile_ = testing_profile_manager_.CreateTestingProfile("me");

    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    file_system_context_ = content::CreateFileSystemContextForTesting(
        nullptr, temp_dir_.GetPath());

    std::vector<std::unique_ptr<RecentSource>> sources;
    sources.emplace_back(CreateFakeRecentSource());
    model_ = base::MakeUnique<RecentModel>(std::move(sources));
    async_file_util_ = base::MakeUnique<RecentAsyncFileUtil>(model_.get());
  }

  static std::unique_ptr<FakeRecentSource> CreateFakeRecentSource() {
    auto source =
        base::MakeUnique<FakeRecentSource>(RecentFile::SourceType::TEST_1);
    source->AddFile("kitten.jpg", "kitten", 28);
    source->AddFile("puppy.jpg", "puppy", 11);
    return source;
  }

  std::unique_ptr<storage::FileSystemOperationContext>
  CreateFileSystemOperationContext() {
    return base::MakeUnique<storage::FileSystemOperationContext>(
        file_system_context_.get());
  }

  storage::FileSystemURL CreateFileSystemURL(const std::string& filename) {
    return storage::FileSystemURL::CreateForTest(
        GURL() /* origin */, storage::kFileSystemTypeRecent,
        GetRecentMountPointPath(profile_).AppendASCII(filename));
  }

  content::TestBrowserThreadBundle thread_bundle_;
  TestingProfileManager testing_profile_manager_;
  Profile* profile_;
  base::ScopedTempDir temp_dir_;
  scoped_refptr<storage::FileSystemContext> file_system_context_;
  std::unique_ptr<RecentModel> model_;
  std::unique_ptr<RecentAsyncFileUtil> async_file_util_;
};

TEST_F(RecentAsyncFileUtilTest, GetFileInfo) {
  base::RunLoop run_loop;

  async_file_util_->GetFileInfo(
      CreateFileSystemOperationContext(), CreateFileSystemURL("kitten.jpg"),
      storage::FileSystemOperation::GET_METADATA_FIELD_SIZE |
          storage::FileSystemOperation::GET_METADATA_FIELD_IS_DIRECTORY,
      base::Bind(
          [](base::RunLoop* run_loop, base::File::Error result,
             const base::File::Info& info) {
            EXPECT_EQ(base::File::FILE_OK, result);
            EXPECT_EQ(28, info.size);
            EXPECT_FALSE(info.is_directory);
            run_loop->Quit();
          },
          &run_loop));

  run_loop.Run();
}

TEST_F(RecentAsyncFileUtilTest, GetFileInfo_NotFound) {
  base::RunLoop run_loop;

  async_file_util_->GetFileInfo(
      CreateFileSystemOperationContext(), CreateFileSystemURL("squid.jpg"),
      storage::FileSystemOperation::GET_METADATA_FIELD_SIZE |
          storage::FileSystemOperation::GET_METADATA_FIELD_IS_DIRECTORY,
      base::Bind(
          [](base::RunLoop* run_loop, base::File::Error result,
             const base::File::Info& info) {
            EXPECT_EQ(base::File::FILE_ERROR_NOT_FOUND, result);
            run_loop->Quit();
          },
          &run_loop));

  run_loop.Run();
}

TEST_F(RecentAsyncFileUtilTest, GetFileInfo_FakeRootEntry) {
  base::RunLoop run_loop;

  async_file_util_->GetFileInfo(
      CreateFileSystemOperationContext(), CreateFileSystemURL(""),
      storage::FileSystemOperation::GET_METADATA_FIELD_SIZE |
          storage::FileSystemOperation::GET_METADATA_FIELD_IS_DIRECTORY,
      base::Bind(
          [](base::RunLoop* run_loop, base::File::Error result,
             const base::File::Info& info) {
            EXPECT_EQ(base::File::FILE_OK, result);
            EXPECT_EQ(-1, info.size);
            EXPECT_TRUE(info.is_directory);
            run_loop->Quit();
          },
          &run_loop));

  run_loop.Run();
}

TEST_F(RecentAsyncFileUtilTest, ReadDirectory) {
  base::RunLoop run_loop;

  async_file_util_->ReadDirectory(
      CreateFileSystemOperationContext(), CreateFileSystemURL(""),
      base::Bind(
          [](base::RunLoop* run_loop, base::File::Error result,
             const storage::AsyncFileUtil::EntryList& entries, bool has_more) {
            EXPECT_EQ(base::File::FILE_OK, result);
            ASSERT_EQ(2u, entries.size());
            EXPECT_EQ("kitten.jpg", entries[0].name);
            EXPECT_FALSE(entries[0].is_directory);
            EXPECT_EQ("puppy.jpg", entries[1].name);
            EXPECT_FALSE(entries[1].is_directory);
            EXPECT_FALSE(has_more);
            run_loop->Quit();
          },
          &run_loop));

  run_loop.Run();
}

TEST_F(RecentAsyncFileUtilTest, ReadDirectory_NonRoot) {
  base::RunLoop run_loop;

  async_file_util_->ReadDirectory(
      CreateFileSystemOperationContext(), CreateFileSystemURL("kitten.jpg"),
      base::Bind(
          [](base::RunLoop* run_loop, base::File::Error result,
             const storage::AsyncFileUtil::EntryList& entries, bool has_more) {
            EXPECT_EQ(base::File::FILE_ERROR_NOT_A_DIRECTORY, result);
            EXPECT_FALSE(has_more);
            run_loop->Quit();
          },
          &run_loop));

  run_loop.Run();
}

}  // namespace
}  // namespace chromeos
