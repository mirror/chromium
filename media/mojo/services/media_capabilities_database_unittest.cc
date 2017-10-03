// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/files/file_path.h"
#include "base/files/scoped_temp_dir.h"
#include "base/logging.h"
#include "base/test/scoped_task_environment.h"
#include "media/base/test_data_util.h"
#include "media/base/video_codecs.h"
#include "media/mojo/services/media_capabilities_database_impl.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/geometry/size.h"

using testing::Pointee;
using testing::Eq;

namespace media {

class MediaCapabilitiesDatabaseTest : public ::testing::Test {
 public:
  MediaCapabilitiesDatabaseTest() = default;

  void SetUp() override {
    // Temporary directory is created/deleted for each test.
    temp_dir_ = std::make_unique<base::ScopedTempDir>();
    ASSERT_TRUE(temp_dir_->CreateUniqueTempDir());

    db_ = std::make_unique<MediaCapabilitiesDatabaseImpl>();

    base::RunLoop run_loop;
    db_->Initialize(
        temp_dir_->GetPath(),
        base::BindOnce(&MediaCapabilitiesDatabaseTest::OnInit,
                       base::Unretained(this), run_loop.QuitClosure()));

    // Let async initialization run.
    run_loop.Run();
  }

  void TearDown() override {
    db_.reset();
    temp_dir_.reset();
  }

  void OnInit(base::OnceClosure init_cb, bool success) {
    ASSERT_TRUE(success);
    std::move(init_cb).Run();
  }

  void ClearDatabaseCache() { db_->cache_.clear(); }

  void ClearInitializedFlag() { db_->db_init_ = false; }

  // Unwraps move-only parameters to pass to the mock function, then runs the
  // RunLoop quit closure.
  void GetDecodingInfoCb(
      base::OnceClosure quit_closure,
      bool success,
      std::unique_ptr<MediaCapabilitiesDatabase::Info> info) {
    MockGetDecodingInfoCb(success, info.get());
    std::move(quit_closure).Run();
  }

  MOCK_METHOD2(MockGetDecodingInfoCb,
               void(bool success, MediaCapabilitiesDatabase::Info* info));

 protected:
  using Entry = MediaCapabilitiesDatabase::Entry;
  using Info = MediaCapabilitiesDatabase::Info;

  base::test::ScopedTaskEnvironment scoped_task_environment_;

  std::unique_ptr<MediaCapabilitiesDatabaseImpl> db_;

  // Temporary directory for DB storage, reset for each test.
  std::unique_ptr<base::ScopedTempDir> temp_dir_;

 private:
  DISALLOW_COPY_AND_ASSIGN(MediaCapabilitiesDatabaseTest);
};

MATCHER_P(InfoEq, other_info, "") {
  return arg.frames_decoded == other_info.frames_decoded &&
         arg.frames_dropped == other_info.frames_dropped;
}

TEST_F(MediaCapabilitiesDatabaseTest, ReadExpectingNothing) {
  MediaCapabilitiesDatabase::Entry entry(VP9PROFILE_PROFILE3,
                                         gfx::Size(1024, 768), 60);

  // Database is empty. Expect null info.
  EXPECT_CALL(*this, MockGetDecodingInfoCb(true, nullptr));

  base::RunLoop run_loop;
  db_->GetDecodingInfo(
      entry, base::BindOnce(&MediaCapabilitiesDatabaseTest::GetDecodingInfoCb,
                            base::Unretained(this), run_loop.QuitClosure()));
  run_loop.Run();
}

TEST_F(MediaCapabilitiesDatabaseTest, WriteAndRead) {
  MediaCapabilitiesDatabase::Entry entry(VP9PROFILE_PROFILE3,
                                         gfx::Size(1024, 768), 60);
  MediaCapabilitiesDatabase::Info info(1000, 2);

  // Add entry/info to the database.
  {
    base::RunLoop run_loop;
    db_->AppendInfoToEntry(entry, info);
    run_loop.RunUntilIdle();
  }

  // Expect to read what was written.
  {
    base::RunLoop run_loop;
    EXPECT_CALL(*this, MockGetDecodingInfoCb(true, Pointee(InfoEq(info))));
    db_->GetDecodingInfo(
        entry, base::BindOnce(&MediaCapabilitiesDatabaseTest::GetDecodingInfoCb,
                              base::Unretained(this), run_loop.QuitClosure()));
    run_loop.Run();
  }

  // Append again to add more frames to that entry's info.
  {
    base::RunLoop run_loop;
    db_->AppendInfoToEntry(entry, info);
    run_loop.RunUntilIdle();
  }

  // Expect to read what was written (2x the initial info).
  MediaCapabilitiesDatabase::Info aggregate_info(2000, 4);
  {
    base::RunLoop run_loop;
    EXPECT_CALL(*this,
                MockGetDecodingInfoCb(true, Pointee(InfoEq(aggregate_info))));
    db_->GetDecodingInfo(
        entry, base::BindOnce(&MediaCapabilitiesDatabaseTest::GetDecodingInfoCb,
                              base::Unretained(this), run_loop.QuitClosure()));
    run_loop.Run();
  }

  // Clear the cache and verify same values come from the DB.
  {
    base::RunLoop run_loop;
    ClearDatabaseCache();
    EXPECT_CALL(*this,
                MockGetDecodingInfoCb(true, Pointee(InfoEq(aggregate_info))));
    db_->GetDecodingInfo(
        entry, base::BindOnce(&MediaCapabilitiesDatabaseTest::GetDecodingInfoCb,
                              base::Unretained(this), run_loop.QuitClosure()));
    run_loop.Run();
  }
}

TEST_F(MediaCapabilitiesDatabaseTest, GetDecodingInfoDbNotInitialized) {
  // Simulate failed/forgotten initialization.
  ClearInitializedFlag();

  MediaCapabilitiesDatabase::Entry entry(VP9PROFILE_PROFILE3,
                                         gfx::Size(1024, 768), 60);
  EXPECT_CALL(*this, MockGetDecodingInfoCb(false, nullptr));
  base::RunLoop run_loop;
  db_->GetDecodingInfo(
      entry, base::BindOnce(&MediaCapabilitiesDatabaseTest::GetDecodingInfoCb,
                            base::Unretained(this), run_loop.QuitClosure()));
  run_loop.Run();
}

}  // namespace media
