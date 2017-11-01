// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/audio/audio_system_tester.h"

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "media/audio/audio_system.h"
#include "media/audio/mock_audio_manager.h"
#include "testing/gmock/include/gmock/gmock.h"

#include "base/threading/thread_task_runner_handle.h"

using ::testing::InvokeWithoutArgs;

namespace media {

namespace {
const char* kNonDefaultDeviceId = "non-default-device-id";
}  // namespace

bool operator==(const AudioDeviceDescription& lhs,
                const AudioDeviceDescription& rhs) {
  return lhs.device_name == rhs.device_name && lhs.unique_id == rhs.unique_id &&
         lhs.group_id == rhs.group_id;
}

AudioSystemTester::AudioSystemTester(MockAudioManager* audio_manager,
                                     AudioSystem* audio_system)
    : audio_manager_(audio_manager),
      audio_system_(audio_system),
      input_params_(AudioParameters::AUDIO_PCM_LINEAR,
                    CHANNEL_LAYOUT_MONO,
                    AudioParameters::kTelephoneSampleRate,
                    16,
                    AudioParameters::kTelephoneSampleRate / 10),
      output_params_(AudioParameters::AUDIO_PCM_LINEAR,
                     CHANNEL_LAYOUT_MONO,
                     AudioParameters::kTelephoneSampleRate,
                     16,
                     AudioParameters::kTelephoneSampleRate / 20),
      default_output_params_(AudioParameters::AUDIO_PCM_LINEAR,
                             CHANNEL_LAYOUT_MONO,
                             AudioParameters::kTelephoneSampleRate,
                             16,
                             AudioParameters::kTelephoneSampleRate / 30) {
  audio_manager_->SetInputStreamParameters(input_params_);
  audio_manager_->SetOutputStreamParameters(output_params_);
  audio_manager_->SetDefaultOutputStreamParameters(default_output_params_);

  auto get_device_descriptions = [](const AudioDeviceDescriptions* source,
                                    AudioDeviceDescriptions* destination) {
    destination->insert(destination->end(), source->begin(), source->end());
  };

  audio_manager_->SetInputDeviceDescriptionsCallback(base::Bind(
      get_device_descriptions, base::Unretained(&input_device_descriptions_)));
  audio_manager_->SetOutputDeviceDescriptionsCallback(base::Bind(
      get_device_descriptions, base::Unretained(&output_device_descriptions_)));
}

AudioSystemTester::~AudioSystemTester() = default;

void AudioSystemTester::TestGetInputStreamParametersNormal() {
  DLOG(ERROR) << "@@@@ " << __PRETTY_FUNCTION__
              << " runner: " << base::ThreadTaskRunnerHandle::Get().get();

  base::RunLoop wait_loop;
  EXPECT_CALL(*this, AudioParametersReceived())
      .WillOnce(InvokeWithoutArgs(&wait_loop, &base::RunLoop::Quit));
  audio_system_->GetInputStreamParameters(
      AudioDeviceDescription::kDefaultDeviceId,
      base::Bind(&AudioSystemTester::OnAudioParams, base::Unretained(this),
                 input_params_));
  wait_loop.Run();
  DLOG(ERROR) << "@@@@ END" << __PRETTY_FUNCTION__
              << " runner: " << base::ThreadTaskRunnerHandle::Get().get();
}

void AudioSystemTester::TestGetInputStreamParametersNoDevice() {
  audio_manager_->SetHasInputDevices(false);

  base::RunLoop wait_loop;
  EXPECT_CALL(*this, AudioParametersReceived())
      .WillOnce(InvokeWithoutArgs(&wait_loop, &base::RunLoop::Quit));
  audio_system_->GetInputStreamParameters(
      AudioDeviceDescription::kDefaultDeviceId,
      base::Bind(&AudioSystemTester::OnAudioParams, base::Unretained(this),
                 base::Optional<AudioParameters>()));
  wait_loop.Run();
}

void AudioSystemTester::TestGetOutputStreamParameters() {
  base::RunLoop wait_loop;
  EXPECT_CALL(*this, AudioParametersReceived())
      .WillOnce(InvokeWithoutArgs(&wait_loop, &base::RunLoop::Quit));
  audio_system_->GetOutputStreamParameters(
      kNonDefaultDeviceId, base::Bind(&AudioSystemTester::OnAudioParams,
                                      base::Unretained(this), output_params_));
  wait_loop.Run();
}

void AudioSystemTester::TestGetDefaultOutputStreamParameters() {
  base::RunLoop wait_loop;
  EXPECT_CALL(*this, AudioParametersReceived())
      .WillOnce(InvokeWithoutArgs(&wait_loop, &base::RunLoop::Quit));
  audio_system_->GetOutputStreamParameters(
      AudioDeviceDescription::kDefaultDeviceId,
      base::Bind(&AudioSystemTester::OnAudioParams, base::Unretained(this),
                 default_output_params_));
  wait_loop.Run();
}

void AudioSystemTester::
    TestGetOutputStreamParametersForDefaultDeviceNoDevices() {
  audio_manager_->SetHasOutputDevices(false);
  base::RunLoop wait_loop;
  EXPECT_CALL(*this, AudioParametersReceived())
      .WillOnce(InvokeWithoutArgs(&wait_loop, &base::RunLoop::Quit));
  audio_system_->GetOutputStreamParameters(
      AudioDeviceDescription::kDefaultDeviceId,
      base::Bind(&AudioSystemTester::OnAudioParams, base::Unretained(this),
                 base::Optional<AudioParameters>()));
  wait_loop.Run();
}

void AudioSystemTester::
    TestGetOutputStreamParametersForNonDefaultDeviceNoDevices() {
  audio_manager_->SetHasOutputDevices(false);
  base::RunLoop wait_loop;
  EXPECT_CALL(*this, AudioParametersReceived())
      .WillOnce(InvokeWithoutArgs(&wait_loop, &base::RunLoop::Quit));
  audio_system_->GetOutputStreamParameters(
      kNonDefaultDeviceId,
      base::Bind(&AudioSystemTester::OnAudioParams, base::Unretained(this),
                 base::Optional<AudioParameters>()));
  wait_loop.Run();
}

void AudioSystemTester::TestHasInputDevices() {
  base::RunLoop wait_loop;
  EXPECT_CALL(*this, HasInputDevicesCallback(true))
      .WillOnce(InvokeWithoutArgs(&wait_loop, &base::RunLoop::Quit));
  audio_system_->HasInputDevices(base::Bind(
      &AudioSystemTester::OnHasInputDevices, base::Unretained(this)));
  wait_loop.Run();
}

void AudioSystemTester::TestHasNoInputDevices() {
  audio_manager_->SetHasInputDevices(false);

  base::RunLoop wait_loop;
  EXPECT_CALL(*this, HasInputDevicesCallback(false))
      .WillOnce(InvokeWithoutArgs(&wait_loop, &base::RunLoop::Quit));
  audio_system_->HasInputDevices(base::Bind(
      &AudioSystemTester::OnHasInputDevices, base::Unretained(this)));
  wait_loop.Run();
}

void AudioSystemTester::TestHasOutputDevices() {
  base::RunLoop wait_loop;
  EXPECT_CALL(*this, HasOutputDevicesCallback(true))
      .WillOnce(InvokeWithoutArgs(&wait_loop, &base::RunLoop::Quit));
  audio_system_->HasOutputDevices(base::Bind(
      &AudioSystemTester::OnHasOutputDevices, base::Unretained(this)));
  wait_loop.Run();
}

void AudioSystemTester::TestHasNoOutputDevices() {
  audio_manager_->SetHasOutputDevices(false);

  base::RunLoop wait_loop;
  EXPECT_CALL(*this, HasOutputDevicesCallback(false))
      .WillOnce(InvokeWithoutArgs(&wait_loop, &base::RunLoop::Quit));
  audio_system_->HasOutputDevices(base::Bind(
      &AudioSystemTester::OnHasOutputDevices, base::Unretained(this)));
  wait_loop.Run();
}

void AudioSystemTester::TestGetInputDeviceDescriptionsNoInputDevices() {
  output_device_descriptions_.emplace_back("output_device_name",
                                           "output_device_id", "group_id");
  EXPECT_EQ(0, static_cast<int>(input_device_descriptions_.size()));
  EXPECT_EQ(1, static_cast<int>(output_device_descriptions_.size()));

  base::RunLoop wait_loop;
  EXPECT_CALL(*this, DeviceDescriptionsReceived())
      .WillOnce(InvokeWithoutArgs(&wait_loop, &base::RunLoop::Quit));
  audio_system_->GetDeviceDescriptions(
      true, base::Bind(&AudioSystemTester::OnGetDeviceDescriptions,
                       base::Unretained(this), input_device_descriptions_));
  wait_loop.Run();
}

void AudioSystemTester::TestGetInputDeviceDescriptions() {
  output_device_descriptions_.emplace_back("output_device_name",
                                           "output_device_id", "group_id");
  input_device_descriptions_.emplace_back("input_device_name1",
                                          "input_device_id1", "group_id1");
  input_device_descriptions_.emplace_back("input_device_name2",
                                          "input_device_id2", "group_id2");
  EXPECT_EQ(2, static_cast<int>(input_device_descriptions_.size()));
  EXPECT_EQ(1, static_cast<int>(output_device_descriptions_.size()));

  base::RunLoop wait_loop;
  EXPECT_CALL(*this, DeviceDescriptionsReceived())
      .WillOnce(InvokeWithoutArgs(&wait_loop, &base::RunLoop::Quit));
  audio_system_->GetDeviceDescriptions(
      true, base::Bind(&AudioSystemTester::OnGetDeviceDescriptions,
                       base::Unretained(this), input_device_descriptions_));
  wait_loop.Run();
}

void AudioSystemTester::TestGetOutputDeviceDescriptionsNoInputDevices() {
  input_device_descriptions_.emplace_back("input_device_name",
                                          "input_device_id", "group_id");
  EXPECT_EQ(0, static_cast<int>(output_device_descriptions_.size()));
  EXPECT_EQ(1, static_cast<int>(input_device_descriptions_.size()));

  base::RunLoop wait_loop;
  EXPECT_CALL(*this, DeviceDescriptionsReceived())
      .WillOnce(InvokeWithoutArgs(&wait_loop, &base::RunLoop::Quit));
  audio_system_->GetDeviceDescriptions(
      false, base::Bind(&AudioSystemTester::OnGetDeviceDescriptions,
                        base::Unretained(this), output_device_descriptions_));
  wait_loop.Run();
}

void AudioSystemTester::TestGetOutputDeviceDescriptions() {
  input_device_descriptions_.emplace_back("input_device_name",
                                          "input_device_id", "group_id");
  output_device_descriptions_.emplace_back("output_device_name1",
                                           "output_device_id1", "group_id1");
  output_device_descriptions_.emplace_back("output_device_name2",
                                           "output_device_id2", "group_id2");
  EXPECT_EQ(2, static_cast<int>(output_device_descriptions_.size()));
  EXPECT_EQ(1, static_cast<int>(input_device_descriptions_.size()));

  base::RunLoop wait_loop;
  EXPECT_CALL(*this, DeviceDescriptionsReceived())
      .WillOnce(InvokeWithoutArgs(&wait_loop, &base::RunLoop::Quit));
  audio_system_->GetDeviceDescriptions(
      false, base::Bind(&AudioSystemTester::OnGetDeviceDescriptions,
                        base::Unretained(this), output_device_descriptions_));
  wait_loop.Run();
}

void AudioSystemTester::TestGetAssociatedOutputDeviceID() {
  const std::string associated_id("associated_id");
  audio_manager_->SetAssociatedOutputDeviceIDCallback(
      base::Bind([](const std::string& result,
                    const std::string&) -> std::string { return result; },
                 associated_id));

  base::RunLoop wait_loop;
  EXPECT_CALL(*this, AssociatedOutputDeviceIDReceived(associated_id))
      .WillOnce(InvokeWithoutArgs(&wait_loop, &base::RunLoop::Quit));
  audio_system_->GetAssociatedOutputDeviceID(
      std::string(),
      base::Bind(&AudioSystemTester::AssociatedOutputDeviceIDReceived,
                 base::Unretained(this)));
  wait_loop.Run();
}

void AudioSystemTester::TestGetInputDeviceInfoNoAssociation() {
  base::RunLoop wait_loop;
  EXPECT_CALL(*this, InputDeviceInfoReceived())
      .WillOnce(InvokeWithoutArgs(&wait_loop, &base::RunLoop::Quit));
  audio_system_->GetInputDeviceInfo(
      kNonDefaultDeviceId,
      base::Bind(&AudioSystemTester::OnInputDeviceInfo, base::Unretained(this),
                 input_params_, base::Optional<AudioParameters>(),
                 std::string()));
  wait_loop.Run();
}

void AudioSystemTester::TestGetInputDeviceInfoWithAssociation() {
  const std::string associated_id("associated_id");
  audio_manager_->SetAssociatedOutputDeviceIDCallback(
      base::Bind([](const std::string& result,
                    const std::string&) -> std::string { return result; },
                 associated_id));

  base::RunLoop wait_loop;
  EXPECT_CALL(*this, InputDeviceInfoReceived())
      .WillOnce(InvokeWithoutArgs(&wait_loop, &base::RunLoop::Quit));
  audio_system_->GetInputDeviceInfo(
      kNonDefaultDeviceId,
      base::Bind(&AudioSystemTester::OnInputDeviceInfo, base::Unretained(this),
                 input_params_, output_params_, associated_id));
  wait_loop.Run();
}

void AudioSystemTester::OnAudioParams(
    const base::Optional<AudioParameters>& expected,
    const base::Optional<AudioParameters>& received) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (expected) {
    EXPECT_TRUE(received);
    EXPECT_EQ(expected->AsHumanReadableString(),
              received->AsHumanReadableString());
  } else {
    EXPECT_FALSE(received);
  }
  AudioParametersReceived();
}

void AudioSystemTester::OnHasInputDevices(bool result) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  HasInputDevicesCallback(result);
}

void AudioSystemTester::OnHasOutputDevices(bool result) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  HasOutputDevicesCallback(result);
}

void AudioSystemTester::OnGetDeviceDescriptions(
    const AudioDeviceDescriptions& expected_descriptions,
    AudioDeviceDescriptions descriptions) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  EXPECT_EQ(expected_descriptions, descriptions);
  DeviceDescriptionsReceived();
}

void AudioSystemTester::OnInputDeviceInfo(
    const base::Optional<AudioParameters>& expected_input,
    const base::Optional<AudioParameters>& expected_associated_output,
    const std::string& expected_associated_device_id,
    const base::Optional<AudioParameters>& input,
    const base::Optional<AudioParameters>& associated_output,
    const std::string& associated_device_id) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (expected_input) {
    EXPECT_TRUE(input);
    EXPECT_EQ(expected_input->AsHumanReadableString(),
              input->AsHumanReadableString());
  } else {
    EXPECT_FALSE(input);
  }
  if (expected_associated_output) {
    EXPECT_TRUE(associated_output);
    EXPECT_EQ(expected_associated_output->AsHumanReadableString(),
              associated_output->AsHumanReadableString());
  } else {
    EXPECT_FALSE(associated_output);
  }
  EXPECT_EQ(expected_associated_device_id, associated_device_id);
  InputDeviceInfoReceived();
}

}  // namespace media
