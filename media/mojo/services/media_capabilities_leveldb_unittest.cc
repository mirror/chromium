// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/files/file_path.h"
#include "base/files/scoped_temp_dir.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/test/scoped_task_environment.h"
#include "media/base/test_data_util.h"
#include "media/base/video_codecs.h"
#include "media/mojo/services/media_capabilities_leveldb.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/geometry/size.h"

using testing::Pointee;
using testing::Eq;

namespace media {

class MediaCapabilitiesLevelDBTest : public ::testing::Test {
 public:
  void SetUp() override {
    // Temporary directory is created/deleted for each test.
    temp_dir_ = base::MakeUnique<base::ScopedTempDir>();
    ASSERT_TRUE(temp_dir_->CreateUniqueTempDir());

    db_ = base::MakeUnique<MediaCapabilitiesLevelDB>(temp_dir_->GetPath());

    // Let async db initialization run.
    scoped_task_environment_.RunUntilIdle();
  }

  void TearDown() override {
    db_.reset();
    temp_dir_.reset();
  }

  void ClearDatabaseCache() { db_->cache_.clear(); }

  // Unwraps move-only parameters to pass to the mock function. Mock functions
  // are not yet able to take move-only parameters.
  void GetInfoCallback(bool success,
                       std::unique_ptr<MediaCapabilitiesDatabase::Info> info) {
    MockGetInfoCallback(success, info.get());
  }

  MOCK_METHOD2(MockGetInfoCallback,
               void(bool success, MediaCapabilitiesDatabase::Info* info));

 protected:
  using Entry = media::MediaCapabilitiesDatabase::Entry;
  using Info = media::MediaCapabilitiesDatabase::Info;

  base::test::ScopedTaskEnvironment scoped_task_environment_;

  std::unique_ptr<MediaCapabilitiesLevelDB> db_;

  // Temporary directory for DB storage, reset for each test.
  std::unique_ptr<base::ScopedTempDir> temp_dir_;
};

MATCHER_P(InfoEq, other_info, "") {
  return arg.frames_decoded == other_info.frames_decoded &&
         arg.frames_dropped == other_info.frames_dropped;
}

TEST_F(MediaCapabilitiesLevelDBTest, ReadExpectingNothing) {
  MediaCapabilitiesDatabase::Entry entry(VP9PROFILE_PROFILE3,
                                         gfx::Size(1024, 768), 60);

  // Database is empty. Expect null info.
  EXPECT_CALL(*this, MockGetInfoCallback(true, nullptr));
  db_->GetInfo(entry,
               base::BindOnce(&MediaCapabilitiesLevelDBTest::GetInfoCallback,
                              base::Unretained(this)));
  scoped_task_environment_.RunUntilIdle();
}

TEST_F(MediaCapabilitiesLevelDBTest, WriteAndRead) {
  MediaCapabilitiesDatabase::Entry entry(VP9PROFILE_PROFILE3,
                                         gfx::Size(1024, 768), 60);
  MediaCapabilitiesDatabase::Info info(1000, 2);

  // Add entry/info to the database.
  db_->AppendInfoToEntry(entry, info);
  scoped_task_environment_.RunUntilIdle();

  // Expect to read what was written.
  EXPECT_CALL(*this, MockGetInfoCallback(true, Pointee(InfoEq(info))));
  db_->GetInfo(entry,
               base::BindOnce(&MediaCapabilitiesLevelDBTest::GetInfoCallback,
                              base::Unretained(this)));
  scoped_task_environment_.RunUntilIdle();

  // Append again to add more frames to that entry's info.
  db_->AppendInfoToEntry(entry, info);
  scoped_task_environment_.RunUntilIdle();

  // Expect to read what was written (2x the initial info).
  MediaCapabilitiesDatabase::Info aggregate_info(2000, 4);
  EXPECT_CALL(*this,
              MockGetInfoCallback(true, Pointee(InfoEq(aggregate_info))));
  db_->GetInfo(entry,
               base::BindOnce(&MediaCapabilitiesLevelDBTest::GetInfoCallback,
                              base::Unretained(this)));
  scoped_task_environment_.RunUntilIdle();

  // Clear the cache and verify same values come from the DB.
  ClearDatabaseCache();
  EXPECT_CALL(*this,
              MockGetInfoCallback(true, Pointee(InfoEq(aggregate_info))));
  db_->GetInfo(entry,
               base::BindOnce(&MediaCapabilitiesLevelDBTest::GetInfoCallback,
                              base::Unretained(this)));
  scoped_task_environment_.RunUntilIdle();
}

}  // namespace media
