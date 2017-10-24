// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/audio/audio_system_impl.h"
#include "base/message_loop/message_loop.h"
#include "media/audio/audio_system_tester.h"
#include "media/audio/audio_thread_impl.h"
#include "media/audio/mock_audio_manager.h"
#include "media/audio/test_audio_thread.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace media {

// TODO(olka): These are the only tests for AudioSystemHelper. Make sure that
// AudioSystemHelper is tested if AudioSystemImpl goes away.
class AudioSystemImplTest : public testing::TestWithParam<bool> {
 public:
  AudioSystemImplTest()
      : use_audio_thread_(GetParam()),
        audio_manager_(std::make_unique<MockAudioManager>(
            std::make_unique<TestAudioThread>(use_audio_thread_))),
        audio_system_(std::make_unique<AudioSystemImpl>(audio_manager_.get())),
        tester_(audio_manager_.get(), audio_system_.get()) {}

  ~AudioSystemImplTest() override { audio_manager_->Shutdown(); }

 protected:
  base::MessageLoop message_loop_;
  bool use_audio_thread_;
  std::unique_ptr<MockAudioManager> audio_manager_;
  std::unique_ptr<AudioSystem> audio_system_;
  AudioSystemTester tester_;
};

#define AUDIO_SYSTEM_TEST(test_name) \
  TEST_P(AudioSystemImplTest, test_name) { tester_.Test##test_name(); }

AUDIO_SYSTEM_TEST(GetInputStreamParametersNormal);
AUDIO_SYSTEM_TEST(GetInputStreamParametersNoDevice);
AUDIO_SYSTEM_TEST(GetOutputStreamParameters);
AUDIO_SYSTEM_TEST(GetDefaultOutputStreamParameters);
AUDIO_SYSTEM_TEST(GetOutputStreamParametersForDefaultDeviceNoDevices);
AUDIO_SYSTEM_TEST(GetOutputStreamParametersForNonDefaultDeviceNoDevices);
AUDIO_SYSTEM_TEST(HasInputDevices);
AUDIO_SYSTEM_TEST(HasNoInputDevices);
AUDIO_SYSTEM_TEST(HasOutputDevices);
AUDIO_SYSTEM_TEST(HasNoOutputDevices);
AUDIO_SYSTEM_TEST(GetInputDeviceDescriptionsNoInputDevices);
AUDIO_SYSTEM_TEST(GetInputDeviceDescriptions);
AUDIO_SYSTEM_TEST(GetOutputDeviceDescriptionsNoInputDevices);
AUDIO_SYSTEM_TEST(GetOutputDeviceDescriptions);
AUDIO_SYSTEM_TEST(GetAssociatedOutputDeviceID);
AUDIO_SYSTEM_TEST(GetInputDeviceInfoNoAssociation);
AUDIO_SYSTEM_TEST(GetInputDeviceInfoWithAssociation);

#undef AUDIO_SYSTEM_TEST

INSTANTIATE_TEST_CASE_P(, AudioSystemImplTest, testing::Values(false, true));

}  // namespace media
