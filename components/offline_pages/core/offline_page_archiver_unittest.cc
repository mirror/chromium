// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/offline_page_archiver.h"

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "components/offline_pages/core/model/offline_page_item_generator.h"
#include "components/offline_pages/core/system_download_manager_stub.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {
const int64_t kDownloadId = 42LL;
}  // namespace

namespace offline_pages {

class OfflinePageArchiverTest : public testing::Test {
 public:
  OfflinePageArchiverTest() {}
  ~OfflinePageArchiverTest() override {}

  SavePageCallback publish_done_callback;

  void SetUp() override;
  void TearDown() override;

  OfflinePageItemGenerator* page_generator() { return &page_generator_; }

  const base::FilePath& temporary_dir_path() {
    return temporary_dir_.GetPath();
  }
  const base::FilePath& private_archive_dir_path() {
    return private_archive_dir_.GetPath();
  }
  const base::FilePath& public_archive_dir_path() {
    return public_archive_dir_.GetPath();
  }

 private:
  base::ScopedTempDir temporary_dir_;
  base::ScopedTempDir private_archive_dir_;
  base::ScopedTempDir public_archive_dir_;
  OfflinePageItemGenerator page_generator_;
};

void OfflinePageArchiverTest::SetUp() {
  ASSERT_TRUE(temporary_dir_.CreateUniqueTempDir());
  ASSERT_TRUE(private_archive_dir_.CreateUniqueTempDir());
  ASSERT_TRUE(public_archive_dir_.CreateUniqueTempDir());
}

void OfflinePageArchiverTest::TearDown() {
  if (temporary_dir_.IsValid()) {
    if (!temporary_dir_.Delete())
      DLOG(ERROR) << "temporary_dir_ not created";
  }
  if (private_archive_dir_.IsValid()) {
    if (!private_archive_dir_.Delete())
      DLOG(ERROR) << "private_persistent_dir not created";
  }
  if (public_archive_dir_.IsValid()) {
    if (!public_archive_dir_.Delete())
      DLOG(ERROR) << "public_archive_dir not created";
  }
}

TEST_F(OfflinePageArchiverTest, PublishPage) {
  // Put an offline page into the private dir, adjust the FilePath
  page_generator()->SetArchiveDirectory(temporary_dir_path());
  OfflinePageItem offline_page = page_generator()->CreateItemWithTempFile();
  base::FilePath old_file_path = offline_page.file_path;
  base::FilePath new_file_path =
      public_archive_dir_path().Append(offline_page.file_path.BaseName());
  std::unique_ptr<SystemDownloadManager> download_manager(
      new SystemDownloadManagerStub(kDownloadId, true));
  scoped_refptr<MoveAndAddResults> move_results = new MoveAndAddResults();

  // Call method under test
  OfflinePageArchiver::PublishPage(offline_page, public_archive_dir_path(),
                                   download_manager.get(), move_results);

  // Check results
  EXPECT_EQ(new_file_path, move_results->new_file_path());
  EXPECT_EQ(SavePageResult::SUCCESS, move_results->move_result());
  EXPECT_EQ(kDownloadId, move_results->download_id());
  // Check there is a file in the new location
  EXPECT_TRUE(
      public_archive_dir_path().IsParent(move_results->new_file_path()));
  EXPECT_TRUE(base::PathExists(move_results->new_file_path()));
  // Check there is no longer a file in the old location
  EXPECT_FALSE(base::PathExists(old_file_path));
}

}  // namespace offline_pages
