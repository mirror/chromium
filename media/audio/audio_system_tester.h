// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_AUDIO_AUDIO_SYSTEM_TESTER_H_
#define MEDIA_AUDIO_AUDIO_SYSTEM_TESTER_H_

#include "base/macros.h"
#include "base/optional.h"
#include "base/threading/thread_checker.h"
#include "media/audio/audio_device_description.h"
#include "media/base/audio_parameters.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace media {
class AudioSystem;
class MockAudioManager;

// Helper class to execute AudioSystem implementation test cases.
class AudioSystemTester {
 public:
  AudioSystemTester(MockAudioManager* audio_manager, AudioSystem* audio_system);
  ~AudioSystemTester();

  void TestGetInputStreamParametersNormal();
  void TestGetInputStreamParametersNoDevice();
  void TestGetOutputStreamParameters();
  void TestGetDefaultOutputStreamParameters();
  void TestGetOutputStreamParametersForDefaultDeviceNoDevices();
  void TestGetOutputStreamParametersForNonDefaultDeviceNoDevices();
  void TestHasInputDevices();
  void TestHasNoInputDevices();
  void TestHasOutputDevices();
  void TestHasNoOutputDevices();
  void TestGetInputDeviceDescriptionsNoInputDevices();
  void TestGetInputDeviceDescriptions();
  void TestGetOutputDeviceDescriptionsNoInputDevices();
  void TestGetOutputDeviceDescriptions();
  void TestGetAssociatedOutputDeviceID();
  void TestGetInputDeviceInfoNoAssociation();
  void TestGetInputDeviceInfoWithAssociation();

 private:
  // Methods to verify correctness of received data.
  void OnAudioParams(const base::Optional<AudioParameters>& expected,
                     const base::Optional<AudioParameters>& received);
  void OnHasInputDevices(bool result);
  void OnHasOutputDevices(bool result);
  void OnGetDeviceDescriptions(
      const AudioDeviceDescriptions& expected_descriptions,
      AudioDeviceDescriptions descriptions);
  void OnInputDeviceInfo(
      const base::Optional<AudioParameters>& expected_input,
      const base::Optional<AudioParameters>& expected_associated_output,
      const std::string& expected_associated_device_id,
      const base::Optional<AudioParameters>& input,
      const base::Optional<AudioParameters>& associated_output,
      const std::string& associated_device_id);

  // Mocks to verify that AudioSystem replied with an expected callback.
  MOCK_METHOD0(AudioParametersReceived, void(void));
  MOCK_METHOD1(HasInputDevicesCallback, void(bool));
  MOCK_METHOD1(HasOutputDevicesCallback, void(bool));
  MOCK_METHOD0(DeviceDescriptionsReceived, void(void));
  MOCK_METHOD1(AssociatedOutputDeviceIDReceived, void(const std::string&));
  MOCK_METHOD0(InputDeviceInfoReceived, void(void));

  THREAD_CHECKER(thread_checker_);

  MockAudioManager* audio_manager_;
  AudioSystem* audio_system_;

  AudioParameters input_params_;
  AudioParameters output_params_;
  AudioParameters default_output_params_;
  AudioDeviceDescriptions input_device_descriptions_;
  AudioDeviceDescriptions output_device_descriptions_;

  DISALLOW_COPY_AND_ASSIGN(AudioSystemTester);
};

}  // namespace media

#endif  // MEDIA_AUDIO_AUDIO_SYSTEM_TESTER_H_
