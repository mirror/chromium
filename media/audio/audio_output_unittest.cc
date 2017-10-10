// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>

#include <memory>

#include "base/base_paths.h"
#include "base/memory/aligned_memory.h"
#include "base/memory/ptr_util.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/sync_socket.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "media/audio/audio_device_info_accessor_for_tests.h"
#include "media/audio/audio_io.h"
#include "media/audio/audio_manager.h"
#include "media/audio/audio_unittest_util.h"
#include "media/audio/mock_audio_source_callback.h"
#include "media/audio/simple_sources.h"
#include "media/audio/test_audio_thread.h"
#include "media/base/limits.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::DoAll;
using ::testing::Field;
using ::testing::Invoke;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::NotNull;
using ::testing::Return;

namespace media {

static constexpr base::TimeDelta kTestStreamDuration =
    base::TimeDelta::FromMilliseconds(250);

// AudioSourceCallback implementations that simulates a source that blocks for
// some time in OnMoreData().
class TestSourceLaggy : public AudioOutputStream::AudioSourceCallback {
 public:
  explicit TestSourceLaggy(base::TimeDelta lag) : lag_(lag) {}

  // AudioSourceCallback interface.
  int OnMoreData(base::TimeDelta delay,
                 base::TimeTicks delay_timestamp,
                 int prior_frames_skipped,
                 AudioBus* dest) override {
    ++callback_count_;
    // Touch the channel memory value to make sure memory is good.
    dest->Zero();

    base::PlatformThread::Sleep(lag_);

    return dest->frames();
  }
  void OnError() override { errors_++; }

  // Returns how many times OnMoreData() has been called.
  int callback_count() const { return callback_count_; }
  // Returns how many times OnError() has been called.
  int errors() const { return errors_; }

 private:
  const base::TimeDelta lag_;

  int callback_count_ = 0;
  int errors_ = 0;
};

class AudioOutputTest : public ::testing::Test {
 public:
  AudioOutputTest() {
    audio_manager_ =
        AudioManager::CreateForTesting(base::MakeUnique<TestAudioThread>());
    audio_manager_device_info_ =
        base::MakeUnique<AudioDeviceInfoAccessorForTests>(audio_manager_.get());
    base::RunLoop().RunUntilIdle();
  }
  ~AudioOutputTest() override { audio_manager_->Shutdown(); }

  // Runs message loop for the specified amount of time.
  void RunMessageLoop(base::TimeDelta delay) {
    base::RunLoop run_loop;
    base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE, run_loop.QuitClosure(), delay);
    run_loop.Run();
  }

 protected:
  base::MessageLoop message_loop_;
  std::unique_ptr<AudioManager> audio_manager_;
  std::unique_ptr<AudioDeviceInfoAccessorForTests> audio_manager_device_info_;
};

// These tests can fail on the build bots when somebody connects to them via
// remote-desktop and the rdp client installs an audio device that fails to open
// at some point, possibly when the connection goes idle.

// Test that can it be created and closed.
TEST_F(AudioOutputTest, GetAndClose) {
  ABORT_AUDIO_TEST_IF_NOT(audio_manager_device_info_->HasAudioOutputDevices());

  AudioOutputStream* oas = audio_manager_->MakeAudioOutputStream(
      AudioParameters(AudioParameters::AUDIO_PCM_LOW_LATENCY,
                      CHANNEL_LAYOUT_STEREO, 8000, 16, 256),
      std::string(), AudioManager::LogCallback());
  ASSERT_NE(nullptr, oas);
  oas->Close();
}

// Test that can it be cannot be created with invalid parameters.
TEST_F(AudioOutputTest, SanityOnMakeParams) {
  ABORT_AUDIO_TEST_IF_NOT(audio_manager_device_info_->HasAudioOutputDevices());

  AudioParameters::Format fmt = AudioParameters::AUDIO_PCM_LOW_LATENCY;
  EXPECT_EQ(nullptr,
            audio_manager_->MakeAudioOutputStream(
                AudioParameters(fmt, CHANNEL_LAYOUT_UNSUPPORTED, 8000, 16, 256),
                std::string(), AudioManager::LogCallback()));

  // Period size is too big.
  EXPECT_EQ(nullptr,
            audio_manager_->MakeAudioOutputStream(
                AudioParameters(fmt, CHANNEL_LAYOUT_MONO, 1024 * 1024, 16, 256),
                std::string(), AudioManager::LogCallback()));

  // 80-bits per sample.
  EXPECT_EQ(nullptr,
            audio_manager_->MakeAudioOutputStream(
                AudioParameters(fmt, CHANNEL_LAYOUT_STEREO, 8000, 80, 256),
                std::string(), AudioManager::LogCallback()));

  // Unsupported channel layout.
  EXPECT_EQ(nullptr,
            audio_manager_->MakeAudioOutputStream(
                AudioParameters(fmt, CHANNEL_LAYOUT_UNSUPPORTED, 8000, 16, 256),
                std::string(), AudioManager::LogCallback()));

  // Negative sample rate.
  EXPECT_EQ(nullptr,
            audio_manager_->MakeAudioOutputStream(
                AudioParameters(fmt, CHANNEL_LAYOUT_STEREO, -8000, 16, 256),
                std::string(), AudioManager::LogCallback()));

  // Negative period size.
  EXPECT_EQ(nullptr,
            audio_manager_->MakeAudioOutputStream(
                AudioParameters(fmt, CHANNEL_LAYOUT_MONO, 8000, 16, -100),
                std::string(), AudioManager::LogCallback()));

  // Zero period size.
  EXPECT_EQ(nullptr, audio_manager_->MakeAudioOutputStream(
                         AudioParameters(fmt, CHANNEL_LAYOUT_MONO, 8000, 16, 0),
                         std::string(), AudioManager::LogCallback()));

  // Packet size is above the maximum.
  EXPECT_EQ(nullptr,
            audio_manager_->MakeAudioOutputStream(
                AudioParameters(fmt, CHANNEL_LAYOUT_MONO, 8000, 16,
                                media::limits::kMaxSamplesPerPacket + 1),
                std::string(), AudioManager::LogCallback()));
}

// Test that it can be opened and closed.
TEST_F(AudioOutputTest, OpenAndClose) {
  ABORT_AUDIO_TEST_IF_NOT(audio_manager_device_info_->HasAudioOutputDevices());

  AudioOutputStream* oas = audio_manager_->MakeAudioOutputStream(
      AudioParameters(AudioParameters::AUDIO_PCM_LOW_LATENCY,
                      CHANNEL_LAYOUT_STEREO, 8000, 16, 256),
      std::string(), AudioManager::LogCallback());
  ASSERT_NE(nullptr, oas);
  EXPECT_TRUE(oas->Open());
  oas->Close();
}

// Run output with slow source that blocks in OnMoreData().
TEST_F(AudioOutputTest, SlowSource) {
  ABORT_AUDIO_TEST_IF_NOT(audio_manager_device_info_->HasAudioOutputDevices());

  AudioOutputStream* oas = audio_manager_->MakeAudioOutputStream(
      AudioParameters(AudioParameters::AUDIO_PCM_LOW_LATENCY,
                      CHANNEL_LAYOUT_MONO, 16000, 16, 256),
      std::string(), AudioManager::LogCallback());
  ASSERT_NE(nullptr, oas);
  TestSourceLaggy test_laggy(base::TimeDelta::FromMilliseconds(90));
  EXPECT_TRUE(oas->Open());
  // The test parameters cause a callback every 32 ms and the source is
  // sleeping for 90 ms, so it is guaranteed that we run out of ready buffers.
  oas->Start(&test_laggy);
  RunMessageLoop(base::TimeDelta::FromMilliseconds(500));
  oas->Stop();
  oas->Close();

  EXPECT_GT(test_laggy.callback_count(), 2);
  EXPECT_FALSE(test_laggy.errors());
}

// Test the case when the thread that calls Start() gets paused. This test is
// best when run over RDP with audio enabled. See crbug.com/19276 for more
// details.
TEST_F(AudioOutputTest, PlaySlowLoop) {
  ABORT_AUDIO_TEST_IF_NOT(audio_manager_device_info_->HasAudioOutputDevices());

  uint32_t samples_100_ms = AudioParameters::kAudioCDSampleRate / 10;
  AudioOutputStream* oas = audio_manager_->MakeAudioOutputStream(
      AudioParameters(AudioParameters::AUDIO_PCM_LOW_LATENCY,
                      CHANNEL_LAYOUT_MONO, AudioParameters::kAudioCDSampleRate,
                      16, samples_100_ms),
      std::string(), AudioManager::LogCallback());
  ASSERT_NE(nullptr, oas);

  SineWaveAudioSource source(1, 200.0, AudioParameters::kAudioCDSampleRate);

  EXPECT_TRUE(oas->Open());
  oas->SetVolume(1.0);

  for (int i = 0; i != 5; ++i) {
    oas->Start(&source);
    base::PlatformThread::Sleep(base::TimeDelta::FromMilliseconds(10));
    oas->Stop();
  }
  oas->Close();

  EXPECT_FALSE(source.errors());
}

// This test produces actual audio for .5 seconds on the default wave
// device at 44.1K s/sec. Parameters have been chosen carefully so you should
// not hear pops or noises while the sound is playing.
TEST_F(AudioOutputTest, Play200HzTone44kHz) {
  if (!audio_manager_device_info_->HasAudioOutputDevices()) {
    LOG(WARNING) << "No output device detected.";
    return;
  }

  uint32_t samples_100_ms = AudioParameters::kAudioCDSampleRate / 10;
  AudioOutputStream* oas = audio_manager_->MakeAudioOutputStream(
      AudioParameters(AudioParameters::AUDIO_PCM_LOW_LATENCY,
                      CHANNEL_LAYOUT_MONO, AudioParameters::kAudioCDSampleRate,
                      16, samples_100_ms),
      std::string(), AudioManager::LogCallback());
  ASSERT_NE(nullptr, oas);

  SineWaveAudioSource source(1, 200.0, AudioParameters::kAudioCDSampleRate);
  source.set_expected_period_size(samples_100_ms);

  EXPECT_TRUE(oas->Open());
  oas->SetVolume(1.0);
  oas->Start(&source);
  RunMessageLoop(kTestStreamDuration);
  oas->Stop();
  oas->Close();

  EXPECT_FALSE(source.errors());
  EXPECT_GE(source.stream_time(), base::TimeDelta());
}

// This test produces actual audio for for .5 seconds on the default wave
// device at 22K s/sec. Parameters have been chosen carefully so you should
// not hear pops or noises while the sound is playing. The audio also should
// sound with a lower volume than Play200HzTone44kHz.
TEST_F(AudioOutputTest, Play200HzTone22kHz) {
  ABORT_AUDIO_TEST_IF_NOT(audio_manager_device_info_->HasAudioOutputDevices());

  uint32_t samples_100_ms = AudioParameters::kAudioCDSampleRate / 20;
  AudioOutputStream* oas = audio_manager_->MakeAudioOutputStream(
      AudioParameters(
          AudioParameters::AUDIO_PCM_LOW_LATENCY, CHANNEL_LAYOUT_MONO,
          AudioParameters::kAudioCDSampleRate / 2, 16, samples_100_ms),
      std::string(), AudioManager::LogCallback());
  ASSERT_NE(nullptr, oas);

  SineWaveAudioSource source(1, 200.0, AudioParameters::kAudioCDSampleRate / 2);
  source.set_expected_period_size(samples_100_ms);

  EXPECT_TRUE(oas->Open());

  oas->SetVolume(0.5);
  oas->Start(&source);
  RunMessageLoop(kTestStreamDuration);

  // Test that the volume is within the set limits.
  double volume = 0.0;
  oas->GetVolume(&volume);
  EXPECT_LT(volume, 0.51);
  EXPECT_GT(volume, 0.49);
  oas->Stop();
  oas->Close();

  EXPECT_FALSE(source.errors());
  EXPECT_GE(source.stream_time(), base::TimeDelta());
}

// This test is to make sure an AudioOutputStream can be started after it was
// stopped. You will here two .25 seconds signal separated by 0.25 seconds
// of silence.
TEST_F(AudioOutputTest, PlayTwice200HzTone44kHz) {
  ABORT_AUDIO_TEST_IF_NOT(audio_manager_device_info_->HasAudioOutputDevices());

  uint32_t samples_100_ms = AudioParameters::kAudioCDSampleRate / 10;
  AudioOutputStream* oas = audio_manager_->MakeAudioOutputStream(
      AudioParameters(AudioParameters::AUDIO_PCM_LOW_LATENCY,
                      CHANNEL_LAYOUT_MONO, AudioParameters::kAudioCDSampleRate,
                      16, samples_100_ms),
      std::string(), AudioManager::LogCallback());
  ASSERT_NE(nullptr, oas);

  SineWaveAudioSource source(1, 200.0, AudioParameters::kAudioCDSampleRate);
  EXPECT_TRUE(oas->Open());
  oas->SetVolume(1.0);

  // Play the sound for kTestStreamDuration.
  oas->Start(&source);
  RunMessageLoop(kTestStreamDuration);
  oas->Stop();

  // Sleep to give silence after stopping the AudioOutputStream.
  RunMessageLoop(kTestStreamDuration);

  // Start again and play for kTestStreamDuration.
  oas->Start(&source);
  RunMessageLoop(kTestStreamDuration);
  oas->Stop();

  oas->Close();

  EXPECT_FALSE(source.errors());
  EXPECT_GE(source.stream_time(), base::TimeDelta());
}

// Low latency mode with with 10ms buffer size.
TEST_F(AudioOutputTest, Play200HzToneLowLatency) {
  ABORT_AUDIO_TEST_IF_NOT(audio_manager_device_info_->HasAudioOutputDevices());

  const AudioParameters params =
      audio_manager_device_info_->GetDefaultOutputStreamParameters();
  int sample_rate = params.sample_rate();

  uint32_t samples_10_ms = sample_rate / 100;
  AudioOutputStream* oas = audio_manager_->MakeAudioOutputStream(
      AudioParameters(AudioParameters::AUDIO_PCM_LOW_LATENCY,
                      CHANNEL_LAYOUT_MONO, sample_rate, 16, samples_10_ms),
      std::string(), AudioManager::LogCallback());
  ASSERT_NE(nullptr, oas);

  SineWaveAudioSource source(1, 200, sample_rate);
  source.set_expected_period_size(samples_10_ms);

  bool opened = oas->Open();
  if (!opened) {
    // It was not possible to open this audio device in mono.
    // No point in continuing the test so let's break here.
    LOG(WARNING) << "Mono is not supported. Skipping test.";
    oas->Close();
    return;
  }
  oas->SetVolume(1.0);

  // Play the wave for .4 seconds.
  oas->Start(&source);
  RunMessageLoop(kTestStreamDuration);
  oas->Stop();
  oas->Close();

  EXPECT_FALSE(source.errors());
  EXPECT_GE(source.stream_time(), base::TimeDelta());
}

}  // namespace media
