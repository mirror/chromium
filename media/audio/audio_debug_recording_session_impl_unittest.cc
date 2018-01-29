// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/audio/audio_debug_recording_session_impl.h"

#include <memory>

#include "base/files/file_util.h"
#include "base/test/scoped_task_environment.h"
#include "media/audio/mock_audio_manager.h"
#include "media/audio/test_audio_thread.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace media {

namespace {
const base::FilePath::CharType kBaseFileName[] =
    FILE_PATH_LITERAL("debug_recording");
const base::FilePath::CharType kInputFileNameExtensions[] =
    FILE_PATH_LITERAL("input.1.wav");
const base::FilePath::CharType kOutputFileNameExtensions[] =
    FILE_PATH_LITERAL("output.1.wav");

void OnFileCreated(base::File debug_file) {}

// Action function called on MockAudioManager::EnableDebugRecording mocked
// method to test |create_file_callback| behavior.
void CreateInputOutputDebugRecordingFiles(
    const AudioDebugRecordingSession::CreateFileCallback&
        create_file_callback) {
  create_file_callback.Run(base::FilePath(kInputFileNameExtensions),
                           base::BindOnce(&OnFileCreated));
  create_file_callback.Run(base::FilePath(kOutputFileNameExtensions),
                           base::BindOnce(&OnFileCreated));
}

}  // namespace

class AudioDebugRecordingSessionImplTest : public testing::Test {
 public:
  AudioDebugRecordingSessionImplTest() {
    base::FilePath temp_dir;
    base::GetTempDir(&temp_dir);
    base_file_path_ = temp_dir.Append(base::FilePath(kBaseFileName));
  }

  void CreateAudioManager() {
    mock_audio_manager_ =
        std::make_unique<MockAudioManager>(std::make_unique<TestAudioThread>());
    ASSERT_NE(nullptr, AudioManager::Get());
    ASSERT_EQ(static_cast<AudioManager*>(mock_audio_manager_.get()),
              AudioManager::Get());
  }

  void ShutdownAudioManager() { ASSERT_TRUE(mock_audio_manager_->Shutdown()); }

  void CreateDebugRecordingSession() {
    audio_debug_recording_session_impl_ =
        std::make_unique<media::AudioDebugRecordingSessionImpl>(
            base_file_path_);
  }

  void DestroyDebugRecordingSession() {
    audio_debug_recording_session_impl_.reset();
  }

 protected:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  std::unique_ptr<MockAudioManager> mock_audio_manager_;
  std::unique_ptr<AudioDebugRecordingSessionImpl>
      audio_debug_recording_session_impl_;
  base::FilePath base_file_path_;
};

TEST_F(AudioDebugRecordingSessionImplTest,
       ConstructorEnablesAndDestructorDisablesDebugRecordingOnAudioManager) {
  ::testing::InSequence seq;

  CreateAudioManager();
  EXPECT_CALL(*mock_audio_manager_, EnableDebugRecording(testing::_));
  CreateDebugRecordingSession();

  EXPECT_CALL(*mock_audio_manager_, DisableDebugRecording());
  DestroyDebugRecordingSession();

  ShutdownAudioManager();
}

TEST_F(AudioDebugRecordingSessionImplTest,
       CreateDestroySessionDontCrashWithNoAudioManager) {
  ASSERT_EQ(nullptr, AudioManager::Get());
  CreateDebugRecordingSession();
  DestroyDebugRecordingSession();
}

TEST_F(AudioDebugRecordingSessionImplTest,
       CreateFileCallbackCreatesExpectedFiles) {
  CreateAudioManager();
  EXPECT_CALL(*mock_audio_manager_, EnableDebugRecording(testing::_))
      .WillOnce(testing::Invoke(CreateInputOutputDebugRecordingFiles));
  CreateDebugRecordingSession();

  // Wait for CreateFileCallback runs to be executed.
  scoped_task_environment_.RunUntilIdle();

  // Check that expected files were created.
  base::FilePath input_recording_filename(
      base_file_path_.AddExtension(kInputFileNameExtensions));
  base::FilePath output_recording_filename(
      base_file_path_.AddExtension(kOutputFileNameExtensions));
  EXPECT_TRUE(base::PathExists(output_recording_filename));
  EXPECT_TRUE(base::PathExists(input_recording_filename));

  // Clean-up
  EXPECT_CALL(*mock_audio_manager_, DisableDebugRecording());
  DestroyDebugRecordingSession();
  ShutdownAudioManager();
  ASSERT_TRUE(base::DeleteFile(output_recording_filename, false));
  ASSERT_TRUE(base::DeleteFile(input_recording_filename, false));
}

}  // namespace media
