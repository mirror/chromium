// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/audio/audio_debug_recording_session.h"
#include "base/test/scoped_task_environment.h"
#include "media/audio/audio_debug_recording_helper.h"
#include "media/audio/mock_audio_manager.h"
#include "media/audio/test_audio_thread.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace media {

class AudioDebugRecordingSessionImplTest : public testing::Test {
 public:
  void SetUp() override {
    base::FilePath file_path("file_path");
    mock_audio_manager_ =
        std::make_unique<MockAudioManager>(std::make_unique<TestAudioThread>());
    EXPECT_CALL(*mock_audio_manager_, EnableDebugRecording(file_path));
    audio_debug_recording_session_impl_ =
        std::make_unique<media::AudioDebugRecordingSessionImpl>(
            mock_audio_manager_.get(), file_path);
  }

  void TearDown() override {
    EXPECT_CALL(*mock_audio_manager_, DisableDebugRecording());
    audio_debug_recording_session_impl_.reset();
    mock_audio_manager_->Shutdown();
  }

 protected:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  std::unique_ptr<MockAudioManager> mock_audio_manager_;
  std::unique_ptr<AudioDebugRecordingSessionImpl>
      audio_debug_recording_session_impl_;
};

TEST_F(AudioDebugRecordingSessionImplTest,
       ConstructorEnablesAndDestructorDisablesDebugRecordingOnAudioManager) {}

}  // namespace media
