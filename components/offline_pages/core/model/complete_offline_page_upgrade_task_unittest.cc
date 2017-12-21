// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/model/complete_offline_page_upgrade_task.h"

#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/memory/ptr_util.h"
#include "base/test/test_mock_time_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/offline_pages/core/model/offline_page_item_generator.h"
#include "components/offline_pages/core/offline_page_metadata_store_test_util.h"
#include "components/offline_pages/core/test_task_runner.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace offline_pages {

namespace {
const char kContentsOfTempFile[] = "Sample content of temp file.";
const char kTargetFileName[] = "target_file_name.mhtml";
const char kTestDigest[] = "TestDigest==";

base::FilePath PrepareTemporaryFile(const base::FilePath& temp_dir) {
  base::FilePath temporary_file_path;
  EXPECT_TRUE(base::CreateTemporaryFileInDir(temp_dir, &temporary_file_path));
  EXPECT_TRUE(
      base::AppendToFile(temporary_file_path,
                         reinterpret_cast<const char*>(&kContentsOfTempFile[0]),
                         sizeof(kContentsOfTempFile)));
  return temporary_file_path;
}

int64_t GetFileSize(const base::FilePath& file) {
  int64_t file_size;
  EXPECT_TRUE(base::GetFileSize(file, &file_size));
  return file_size;
}

}  // namespace

class CompleteOfflinePageUpgradeTaskTest : public testing::Test {
 public:
  CompleteOfflinePageUpgradeTaskTest();
  ~CompleteOfflinePageUpgradeTaskTest() override;

  void SetUp() override;
  void TearDown() override;

  void CompleteUpgradeDone(CompleteUpgradeStatus result);

  OfflinePageMetadataStoreSQL* store() { return store_test_util_.store(); }
  OfflinePageMetadataStoreTestUtil* store_test_util() {
    return &store_test_util_;
  }
  OfflinePageItemGenerator* generator() { return &generator_; }
  TestTaskRunner* runner() { return &runner_; }

  CompleteUpgradeCallback callback() {
    return base::BindOnce(
        &CompleteOfflinePageUpgradeTaskTest::CompleteUpgradeDone,
        base::Unretained(this));
  }

  CompleteUpgradeStatus last_result() const { return last_result_; }

 private:
  scoped_refptr<base::TestMockTimeTaskRunner> task_runner_;
  base::ThreadTaskRunnerHandle task_runner_handle_;
  OfflinePageMetadataStoreTestUtil store_test_util_;
  OfflinePageItemGenerator generator_;
  TestTaskRunner runner_;
  CompleteUpgradeStatus last_result_;
};

CompleteOfflinePageUpgradeTaskTest::CompleteOfflinePageUpgradeTaskTest()
    : task_runner_(new base::TestMockTimeTaskRunner()),
      task_runner_handle_(task_runner_),
      store_test_util_(task_runner_),
      runner_(task_runner_),
      last_result_(CompleteUpgradeStatus::DB_ERROR) {}

CompleteOfflinePageUpgradeTaskTest::~CompleteOfflinePageUpgradeTaskTest() {}

void CompleteOfflinePageUpgradeTaskTest::SetUp() {
  store_test_util_.BuildStoreInMemory();
}

void CompleteOfflinePageUpgradeTaskTest::TearDown() {
  store_test_util_.DeleteStore();
}

void CompleteOfflinePageUpgradeTaskTest::CompleteUpgradeDone(
    CompleteUpgradeStatus result) {
  last_result_ = result;
}

TEST_F(CompleteOfflinePageUpgradeTaskTest, CompleteUpgradeSuccess) {
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  generator()->SetArchiveDirectory(temp_dir.GetPath());

  OfflinePageItem page = generator()->CreateItemWithTempFile();
  page.upgrade_attempt = 3;
  store_test_util()->InsertItem(page);

  base::FilePath temporary_file_path = PrepareTemporaryFile(temp_dir.GetPath());
  int64_t file_size = GetFileSize(temporary_file_path);

  base::FilePath target_file_path =
      temp_dir.GetPath().AppendASCII(kTargetFileName);

  // TODO(fgorski): Replace test with actually computed digest.
  auto task = base::MakeUnique<CompleteOfflinePageUpgradeTask>(
      store(), page.offline_id, temporary_file_path, target_file_path,
      kTestDigest, file_size, callback());
  runner()->RunTask(std::move(task));

  EXPECT_EQ(CompleteUpgradeStatus::SUCCESS, last_result());

  auto offline_page = store_test_util()->GetPageByOfflineId(page.offline_id);
  ASSERT_TRUE(offline_page);
  EXPECT_EQ(0, offline_page->upgrade_attempt);
  EXPECT_EQ(target_file_path, offline_page->file_path);
  EXPECT_EQ(file_size, offline_page->file_size);
  EXPECT_EQ(kTestDigest, offline_page->digest);
  EXPECT_FALSE(base::PathExists(page.file_path));
  EXPECT_FALSE(base::PathExists(temporary_file_path));
}

TEST_F(CompleteOfflinePageUpgradeTaskTest, CompleteUpgradeItemMissing) {
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());

  base::FilePath temporary_file_path = PrepareTemporaryFile(temp_dir.GetPath());
  int64_t file_size = GetFileSize(temporary_file_path);

  base::FilePath target_file_path =
      temp_dir.GetPath().AppendASCII(kTargetFileName);

  auto task = base::MakeUnique<CompleteOfflinePageUpgradeTask>(
      store(), 42, temporary_file_path, target_file_path, kTestDigest,
      file_size, callback());
  runner()->RunTask(std::move(task));

  EXPECT_EQ(CompleteUpgradeStatus::ITEM_MISSING, last_result());
}

TEST_F(CompleteOfflinePageUpgradeTaskTest,
       CompleteUpgradeTemporaryFileMissing) {
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  generator()->SetArchiveDirectory(temp_dir.GetPath());

  OfflinePageItem page = generator()->CreateItemWithTempFile();
  page.upgrade_attempt = 3;
  store_test_util()->InsertItem(page);

  base::FilePath temporary_file_path = PrepareTemporaryFile(temp_dir.GetPath());
  int64_t file_size = GetFileSize(temporary_file_path);

  // This ensure the file won't be there.
  EXPECT_TRUE(base::DeleteFile(temporary_file_path, false));

  base::FilePath target_file_path =
      temp_dir.GetPath().AppendASCII(kTargetFileName);

  // TODO(fgorski): Replace test with actually computed digest.
  auto task = base::MakeUnique<CompleteOfflinePageUpgradeTask>(
      store(), page.offline_id, temporary_file_path, target_file_path,
      kTestDigest, file_size, callback());
  runner()->RunTask(std::move(task));

  EXPECT_EQ(CompleteUpgradeStatus::TEMPORARY_FILE_MISSING, last_result());
}

TEST_F(CompleteOfflinePageUpgradeTaskTest, CompelteUpgradeTargetFileNameInUse) {
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  generator()->SetArchiveDirectory(temp_dir.GetPath());

  OfflinePageItem page = generator()->CreateItemWithTempFile();
  page.upgrade_attempt = 3;
  store_test_util()->InsertItem(page);

  base::FilePath temporary_file_path = PrepareTemporaryFile(temp_dir.GetPath());
  int64_t file_size = GetFileSize(temporary_file_path);

  base::FilePath target_file_path =
      temp_dir.GetPath().AppendASCII(kTargetFileName);

  // This ensures target name is taken.
  EXPECT_TRUE(base::CopyFile(temporary_file_path, target_file_path));

  // TODO(fgorski): Replace test with actually computed digest.
  auto task = base::MakeUnique<CompleteOfflinePageUpgradeTask>(
      store(), page.offline_id, temporary_file_path, target_file_path,
      kTestDigest, file_size, callback());
  runner()->RunTask(std::move(task));

  EXPECT_EQ(CompleteUpgradeStatus::TARGET_FILE_NAME_IN_USE, last_result());
}

}  // namespace offline_pages
