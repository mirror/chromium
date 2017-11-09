// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/audio/audio_system_tester.h"

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "media/audio/mock_audio_manager.h"
#include "testing/gmock/include/gmock/gmock.h"

using ::testing::InvokeWithoutArgs;

namespace media {

bool operator==(const AudioDeviceDescription& lhs,
                const AudioDeviceDescription& rhs) {
  return lhs.device_name == rhs.device_name && lhs.unique_id == rhs.unique_id &&
         lhs.group_id == rhs.group_id;
}

AudioSystem::OnAudioParamsCallback
AudioSystemCallbackExpectations::GetAudioParamsCallback(
    const base::Location& location,
    base::OnceClosure on_cb_received,
    const base::Optional<AudioParameters>& expected_params) {
  return base::BindOnce(&AudioSystemCallbackExpectations::OnAudioParams,
                        base::Unretained(this), location.ToString(),
                        std::move(on_cb_received), expected_params);
}

AudioSystem::OnBoolCallback AudioSystemCallbackExpectations::GetBoolCallback(
    const base::Location& location,
    base::OnceClosure on_cb_received,
    bool expected) {
  return base::BindOnce(&AudioSystemCallbackExpectations::OnBool,
                        base::Unretained(this), location.ToString(),
                        std::move(on_cb_received), expected);
}

AudioSystem::OnDeviceDescriptionsCallback
AudioSystemCallbackExpectations::GetDeviceDescriptionsCallback(
    const base::Location& location,
    base::OnceClosure on_cb_received,
    const AudioDeviceDescriptions& expected_descriptions) {
  return base::BindOnce(&AudioSystemCallbackExpectations::OnDeviceDescriptions,
                        base::Unretained(this), location.ToString(),
                        std::move(on_cb_received), expected_descriptions);
}

AudioSystem::OnInputDeviceInfoCallback
AudioSystemCallbackExpectations::GetInputDeviceInfoCallback(
    const base::Location& location,
    base::OnceClosure on_cb_received,
    const base::Optional<AudioParameters>& expected_input,
    const base::Optional<AudioParameters>& expected_associated_output,
    const std::string& expected_associated_device_id) {
  return base::BindOnce(&AudioSystemCallbackExpectations::OnInputDeviceInfo,
                        base::Unretained(this), location.ToString(),
                        std::move(on_cb_received), expected_input,
                        expected_associated_output,
                        expected_associated_device_id);
}

AudioSystem::OnDeviceIdCallback
AudioSystemCallbackExpectations::GetDeviceIdCallback(
    const base::Location& location,
    base::OnceClosure on_cb_received,
    const std::string& expected_id) {
  return base::BindOnce(&AudioSystemCallbackExpectations::OnDeviceId,
                        base::Unretained(this), location.ToString(),
                        std::move(on_cb_received), expected_id);
}

void AudioSystemCallbackExpectations::OnAudioParams(
    const std::string& from_here,
    base::OnceClosure on_cb_received,
    const base::Optional<AudioParameters>& expected,
    const base::Optional<AudioParameters>& received) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_) << from_here;
  if (expected) {
    EXPECT_TRUE(received) << from_here;
    EXPECT_EQ(expected->AsHumanReadableString(),
              received->AsHumanReadableString())
        << from_here;
  } else {
    EXPECT_FALSE(received) << from_here;
  }
  std::move(on_cb_received).Run();
}

void AudioSystemCallbackExpectations::OnBool(const std::string& from_here,
                                             base::OnceClosure on_cb_received,
                                             bool expected,
                                             bool result) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_) << from_here;
  EXPECT_EQ(expected, result) << from_here;
  std::move(on_cb_received).Run();
}

void AudioSystemCallbackExpectations::OnDeviceDescriptions(
    const std::string& from_here,
    base::OnceClosure on_cb_received,
    const AudioDeviceDescriptions& expected_descriptions,
    AudioDeviceDescriptions descriptions) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_) << from_here;
  EXPECT_EQ(expected_descriptions, descriptions);
  std::move(on_cb_received).Run();
}

void AudioSystemCallbackExpectations::OnInputDeviceInfo(
    const std::string& from_here,
    base::OnceClosure on_cb_received,
    const base::Optional<AudioParameters>& expected_input,
    const base::Optional<AudioParameters>& expected_associated_output,
    const std::string& expected_associated_device_id,
    const base::Optional<AudioParameters>& input,
    const base::Optional<AudioParameters>& associated_output,
    const std::string& associated_device_id) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_) << from_here;
  if (expected_input) {
    EXPECT_TRUE(input) << from_here;
    EXPECT_EQ(expected_input->AsHumanReadableString(),
              input->AsHumanReadableString())
        << from_here;
  } else {
    EXPECT_FALSE(input) << from_here;
  }
  if (expected_associated_output) {
    EXPECT_TRUE(associated_output) << from_here;
    EXPECT_EQ(expected_associated_output->AsHumanReadableString(),
              associated_output->AsHumanReadableString())
        << from_here;
  } else {
    EXPECT_FALSE(associated_output) << from_here;
  }
  EXPECT_EQ(expected_associated_device_id, associated_device_id) << from_here;
  std::move(on_cb_received).Run();
}

void AudioSystemCallbackExpectations::OnDeviceId(
    const std::string& from_here,
    base::OnceClosure on_cb_received,
    const std::string& expected_id,
    const std::string& result_id) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_) << from_here;
  EXPECT_EQ(expected_id, result_id) << from_here;
  std::move(on_cb_received).Run();
}

namespace {
const char* kNonDefaultDeviceId = "non-default-device-id";
}  // namespace

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
  base::RunLoop wait_loop;
  audio_system_->GetInputStreamParameters(
      AudioDeviceDescription::kDefaultDeviceId,
      expectations_.GetAudioParamsCallback(FROM_HERE, wait_loop.QuitClosure(),
                                           input_params_));
  wait_loop.Run();
}

void AudioSystemTester::TestGetInputStreamParametersNoDevice() {
  audio_manager_->SetHasInputDevices(false);

  base::RunLoop wait_loop;
  audio_system_->GetInputStreamParameters(
      AudioDeviceDescription::kDefaultDeviceId,
      expectations_.GetAudioParamsCallback(FROM_HERE, wait_loop.QuitClosure(),
                                           base::Optional<AudioParameters>()));
  wait_loop.Run();
}

void AudioSystemTester::TestGetOutputStreamParameters() {
  base::RunLoop wait_loop;
  audio_system_->GetOutputStreamParameters(
      kNonDefaultDeviceId,
      expectations_.GetAudioParamsCallback(FROM_HERE, wait_loop.QuitClosure(),
                                           output_params_));
  wait_loop.Run();
}

void AudioSystemTester::TestGetDefaultOutputStreamParameters() {
  base::RunLoop wait_loop;
  audio_system_->GetOutputStreamParameters(
      AudioDeviceDescription::kDefaultDeviceId,
      expectations_.GetAudioParamsCallback(FROM_HERE, wait_loop.QuitClosure(),
                                           default_output_params_));
  wait_loop.Run();
}

void AudioSystemTester::
    TestGetOutputStreamParametersForDefaultDeviceNoDevices() {
  audio_manager_->SetHasOutputDevices(false);
  base::RunLoop wait_loop;
  audio_system_->GetOutputStreamParameters(
      AudioDeviceDescription::kDefaultDeviceId,
      expectations_.GetAudioParamsCallback(FROM_HERE, wait_loop.QuitClosure(),
                                           base::Optional<AudioParameters>()));
  wait_loop.Run();
}

void AudioSystemTester::
    TestGetOutputStreamParametersForNonDefaultDeviceNoDevices() {
  audio_manager_->SetHasOutputDevices(false);
  base::RunLoop wait_loop;
  audio_system_->GetOutputStreamParameters(
      kNonDefaultDeviceId,
      expectations_.GetAudioParamsCallback(FROM_HERE, wait_loop.QuitClosure(),
                                           base::Optional<AudioParameters>()));
  wait_loop.Run();
}

void AudioSystemTester::TestHasInputDevices() {
  base::RunLoop wait_loop;
  audio_system_->HasInputDevices(
      expectations_.GetBoolCallback(FROM_HERE, wait_loop.QuitClosure(), true));
  wait_loop.Run();
}

void AudioSystemTester::TestHasNoInputDevices() {
  audio_manager_->SetHasInputDevices(false);
  base::RunLoop wait_loop;
  audio_system_->HasInputDevices(
      expectations_.GetBoolCallback(FROM_HERE, wait_loop.QuitClosure(), false));
  wait_loop.Run();
}

void AudioSystemTester::TestHasOutputDevices() {
  base::RunLoop wait_loop;
  audio_system_->HasOutputDevices(
      expectations_.GetBoolCallback(FROM_HERE, wait_loop.QuitClosure(), true));
  wait_loop.Run();
}

void AudioSystemTester::TestHasNoOutputDevices() {
  audio_manager_->SetHasOutputDevices(false);
  base::RunLoop wait_loop;
  audio_system_->HasOutputDevices(
      expectations_.GetBoolCallback(FROM_HERE, wait_loop.QuitClosure(), false));
  wait_loop.Run();
}

void AudioSystemTester::TestGetInputDeviceDescriptionsNoInputDevices() {
  output_device_descriptions_.emplace_back("output_device_name",
                                           "output_device_id", "group_id");
  EXPECT_EQ(0, static_cast<int>(input_device_descriptions_.size()));
  EXPECT_EQ(1, static_cast<int>(output_device_descriptions_.size()));

  base::RunLoop wait_loop;
  audio_system_->GetDeviceDescriptions(
      true,
      expectations_.GetDeviceDescriptionsCallback(
          FROM_HERE, wait_loop.QuitClosure(), input_device_descriptions_));
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
  audio_system_->GetDeviceDescriptions(
      true,
      expectations_.GetDeviceDescriptionsCallback(
          FROM_HERE, wait_loop.QuitClosure(), input_device_descriptions_));
  wait_loop.Run();
}

void AudioSystemTester::TestGetOutputDeviceDescriptionsNoInputDevices() {
  input_device_descriptions_.emplace_back("input_device_name",
                                          "input_device_id", "group_id");
  EXPECT_EQ(0, static_cast<int>(output_device_descriptions_.size()));
  EXPECT_EQ(1, static_cast<int>(input_device_descriptions_.size()));

  base::RunLoop wait_loop;
  audio_system_->GetDeviceDescriptions(
      false,
      expectations_.GetDeviceDescriptionsCallback(
          FROM_HERE, wait_loop.QuitClosure(), output_device_descriptions_));
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
  audio_system_->GetDeviceDescriptions(
      false,
      expectations_.GetDeviceDescriptionsCallback(
          FROM_HERE, wait_loop.QuitClosure(), output_device_descriptions_));
  wait_loop.Run();
}

void AudioSystemTester::TestGetAssociatedOutputDeviceID() {
  const std::string associated_id("associated_id");
  audio_manager_->SetAssociatedOutputDeviceIDCallback(
      base::Bind([](const std::string& result,
                    const std::string&) -> std::string { return result; },
                 associated_id));

  base::RunLoop wait_loop;
  audio_system_->GetAssociatedOutputDeviceID(
      std::string(), expectations_.GetDeviceIdCallback(
                         FROM_HERE, wait_loop.QuitClosure(), associated_id));
  wait_loop.Run();
}

void AudioSystemTester::TestGetInputDeviceInfoNoAssociation() {
  base::RunLoop wait_loop;
  audio_system_->GetInputDeviceInfo(
      kNonDefaultDeviceId,
      expectations_.GetInputDeviceInfoCallback(
          FROM_HERE, wait_loop.QuitClosure(), input_params_,
          base::Optional<AudioParameters>(), std::string()));
  wait_loop.Run();
}

void AudioSystemTester::TestGetInputDeviceInfoWithAssociation() {
  const std::string associated_id("associated_id");
  audio_manager_->SetAssociatedOutputDeviceIDCallback(
      base::Bind([](const std::string& result,
                    const std::string&) -> std::string { return result; },
                 associated_id));

  base::RunLoop wait_loop;
  audio_system_->GetInputDeviceInfo(
      kNonDefaultDeviceId, expectations_.GetInputDeviceInfoCallback(
                               FROM_HERE, wait_loop.QuitClosure(),
                               input_params_, output_params_, associated_id));
  wait_loop.Run();
}

}  // namespace media
