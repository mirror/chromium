// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/in_progress_metadata/in_progress_metadata_cache.h"

#include "base/task_scheduler/post_task.h"
#include "base/task_scheduler/task_traits.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;

namespace active_downloads {
namespace {

class InProgressMetadataCacheTest : public testing::Test {
 public:
  InProgressMetadataCacheTest() {}
  ~InProgressMetadataCacheTest() override = default;

  void SetUp() override {
    file_path_ =
        profile_.GetPath().Append(FILE_PATH_LITERAL("InProgressMetadataCache"));
    task_runner_ = base::CreateSequencedTaskRunnerWithTraits(
        {base::TaskPriority::BACKGROUND,
         base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN, base::MayBlock()});
    cache_.reset(new InProgressMetadataCache(file_path_, task_runner_));
  }

 protected:
  std::unique_ptr<active_downloads::InProgressMetadataCache> cache_;
  base::FilePath file_path_;
  scoped_refptr<base::SequencedTaskRunner> task_runner_;

  TestingProfile profile_;

 private:
  DISALLOW_COPY_AND_ASSIGN(InProgressMetadataCacheTest);
};  // class InProgressMetadataCacheTest

}  // namespace

TEST_F(InProgressMetadataCacheTest, SuccessfulWriteAndRead) {
  std::string test_guid = "test guid";
  DownloadEntry test_entry;
  test_entry.guid = test_guid;

  cache_->AddOrUpdateMetadata(test_entry);
  DownloadEntry retrieved_entry = cache_->RetrieveMetadata(test_guid);
  EXPECT_EQ(test_entry, retrieved_entry);
}

}  // namespace active_downloads
