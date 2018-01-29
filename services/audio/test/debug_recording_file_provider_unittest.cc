// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include <memory>

#include "base/files/file_util.h"
#include "base/test/scoped_task_environment.h"
#include "services/audio/public/cpp/debug_recording_file_provider.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace audio {

namespace {

const base::FilePath::CharType kBaseFileName[] =
    FILE_PATH_LITERAL("debug_recording");
const base::FilePath::CharType kValidSuffix[] = FILE_PATH_LITERAL("_1.wav");
const base::FilePath::CharType kEmptySuffix[] = FILE_PATH_LITERAL("");
const base::FilePath::CharType kInvalidSuffix[] =
    FILE_PATH_LITERAL("/../invalid");

}  // namespace

class DebugRecordingFileProviderTest : public testing::Test {
 public:
  void SetUp() override {
    base::FilePath temp_dir;
    ASSERT_TRUE(base::GetTempDir(&temp_dir));
    file_path_ = temp_dir.Append(kBaseFileName);
    file_provider_ = std::make_unique<DebugRecordingFileProvider>(
        mojo::MakeRequest(&file_provider_ptr_), file_path_);
    ASSERT_TRUE(file_provider_ptr_.is_bound());
  }

  void TearDown() override { file_provider_.reset(); }

  base::FilePath GetFileName(base::FilePath suffix) {
    return file_path_.InsertBeforeExtension(suffix.value());
  }

  MOCK_METHOD1(OnFileCreated, void(bool));
  void FileCreated(base::File file) { OnFileCreated(file.IsValid()); }

 protected:
  std::unique_ptr<DebugRecordingFileProvider> file_provider_;
  mojom::DebugRecordingFileProviderPtr file_provider_ptr_;
  base::FilePath file_path_;
  // The test task environment.
  base::test::ScopedTaskEnvironment scoped_task_environment_;
};

TEST_F(DebugRecordingFileProviderTest, CreateReturnsValidFile) {
  base::FilePath suffix(kValidSuffix);
  EXPECT_CALL(*this, OnFileCreated(true));
  file_provider_ptr_->Create(
      suffix, base::BindOnce(&DebugRecordingFileProviderTest::FileCreated,
                             base::Unretained(this)));
  scoped_task_environment_.RunUntilIdle();

  base::FilePath file_name(GetFileName(suffix));
  EXPECT_TRUE(base::PathExists(file_name));
  ASSERT_TRUE(base::DeleteFile(file_name, false));
}

TEST_F(DebugRecordingFileProviderTest, CreateWithEmptySuffixReturnsValidFile) {
  base::FilePath suffix(kEmptySuffix);
  EXPECT_CALL(*this, OnFileCreated(true));
  file_provider_ptr_->Create(
      suffix, base::BindOnce(&DebugRecordingFileProviderTest::FileCreated,
                             base::Unretained(this)));
  scoped_task_environment_.RunUntilIdle();

  base::FilePath file_name(GetFileName(suffix));
  EXPECT_TRUE(base::PathExists(file_name));
  ASSERT_TRUE(base::DeleteFile(file_name, false));
}

TEST_F(DebugRecordingFileProviderTest,
       CreateWithInvalidSuffixReturnsInvalidFile) {
  base::FilePath suffix(kInvalidSuffix);
  EXPECT_CALL(*this, OnFileCreated(false));
  file_provider_ptr_->Create(
      suffix, base::BindOnce(&DebugRecordingFileProviderTest::FileCreated,
                             base::Unretained(this)));
  scoped_task_environment_.RunUntilIdle();

  EXPECT_FALSE(base::PathExists(GetFileName(suffix)));
}

}  // namespace audio
