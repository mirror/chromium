// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_AUDIO_AUDIO_SYSTEM_TESTER_H_
#define MEDIA_AUDIO_AUDIO_SYSTEM_TESTER_H_

#include "base/location.h"
#include "base/macros.h"
#include "base/optional.h"
#include "base/threading/thread_checker.h"
#include "media/audio/audio_device_description.h"
#include "media/audio/audio_system.h"
#include "media/base/audio_parameters.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace media {
class AudioSystem;
class MockAudioManager;

// Creates AudioSystem callbacks to be passed to AudioSystem methods. When
// AudioSystem calls such a callback, it verifies treading expectations and
// checks recieved parameters against expectated values passed during its
// creation. After that it calls |on_cb_received| closure.
// Note AudioSystemCallbackExpectations object must outlive all the callbacks
// it produced, since they contain raw pointers to it.
class AudioSystemCallbackExpectations {
 public:
  AudioSystemCallbackExpectations() = default;
  AudioSystem::OnAudioParamsCallback GetAudioParamsCallback(
      const base::Location& location,
      base::OnceClosure on_cb_received,
      const base::Optional<AudioParameters>& expected_params);

  AudioSystem::OnBoolCallback GetBoolCallback(const base::Location& location,
                                              base::OnceClosure on_cb_received,
                                              bool expected);

  AudioSystem::OnDeviceDescriptionsCallback GetDeviceDescriptionsCallback(
      const base::Location& location,
      base::OnceClosure on_cb_received,
      const AudioDeviceDescriptions& expected_descriptions);

  AudioSystem::OnInputDeviceInfoCallback GetInputDeviceInfoCallback(
      const base::Location& location,
      base::OnceClosure on_cb_received,
      const base::Optional<AudioParameters>& expected_input,
      const base::Optional<AudioParameters>& expected_associated_output,
      const std::string& expected_associated_device_id);

  AudioSystem::OnDeviceIdCallback GetDeviceIdCallback(
      const base::Location& location,
      base::OnceClosure on_cb_received,
      const std::string& expected_id);

 private:
  // Methods to verify correctness of received data.
  void OnAudioParams(const std::string& from_here,
                     base::OnceClosure on_cb_received,
                     const base::Optional<AudioParameters>& expected,
                     const base::Optional<AudioParameters>& received);

  void OnBool(const std::string& from_here,
              base::OnceClosure on_cb_received,
              bool expected,
              bool result);

  void OnDeviceDescriptions(
      const std::string& from_here,
      base::OnceClosure on_cb_received,
      const AudioDeviceDescriptions& expected_descriptions,
      AudioDeviceDescriptions descriptions);

  void OnInputDeviceInfo(
      const std::string& from_here,
      base::OnceClosure on_cb_received,
      const base::Optional<AudioParameters>& expected_input,
      const base::Optional<AudioParameters>& expected_associated_output,
      const std::string& expected_associated_device_id,
      const base::Optional<AudioParameters>& input,
      const base::Optional<AudioParameters>& associated_output,
      const std::string& associated_device_id);

  void OnDeviceId(const std::string& from_here,
                  base::OnceClosure on_cb_received,
                  const std::string& expected_id,
                  const std::string& result_id);

  THREAD_CHECKER(thread_checker_);
  DISALLOW_COPY_AND_ASSIGN(AudioSystemCallbackExpectations);
};

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
  AudioSystemCallbackExpectations expectations_;
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
