// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_AUDIO_AUDIO_SYSTEM_HELPER_H_
#define MEDIA_AUDIO_AUDIO_SYSTEM_HELPER_H_

#include "media/audio/audio_system.h"
#include "media/base/media_export.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace media {
class AudioManager;

// Helper class wrapping AudioManager functionality. Methods to be called on
// audio thread only. Only audio system implementations are allowed to use it.
class MEDIA_EXPORT AudioSystemHelper {
 public:
  AudioSystemHelper(AudioManager* audio_manager);
  ~AudioSystemHelper();

  // Callback may receive invalid parameters, it means the specified device is
  // not found. This is best-effort: valid parameters do not guarantee existence
  // of the device.
  // TODO(olka,tommi): fix all AudioManager implementations to return invalid
  // parameters if the device is not found.
  void GetInputStreamParameters(
      const std::string& device_id,
      AudioSystem::OnAudioParamsCallback on_params_cb);

  // If media::AudioDeviceDescription::IsDefaultDevice(device_id) is true,
  // callback will receive the parameters of the default output device.
  // Callback may receive invalid parameters, it means the specified device is
  // not found. This is best-effort: valid parameters do not guarantee existence
  // of the device.
  // TODO(olka,tommi): fix all AudioManager implementations to return invalid
  // parameters if the device is not found.
  void GetOutputStreamParameters(
      const std::string& device_id,
      AudioSystem::OnAudioParamsCallback on_params_cb);

  void HasInputDevices(AudioSystem::OnBoolCallback on_has_devices_cb);

  void HasOutputDevices(AudioSystem::OnBoolCallback on_has_devices_cb);

  // Replies with device descriptions of input audio devices if |for_input| is
  // true, and of output audio devices otherwise.
  void GetDeviceDescriptions(
      bool for_input,
      AudioSystem::OnDeviceDescriptionsCallback on_descriptions_cp);

  // Replies with an empty string if there is no associated output device found.
  void GetAssociatedOutputDeviceID(
      const std::string& input_device_id,
      AudioSystem::OnDeviceIdCallback on_device_id_cb);

  // Replies with audio parameters for the specified input device and audio
  // parameters and device ID of the associated output device, if any (otherwise
  // it's AudioParameters() and an empty string).
  void GetInputDeviceInfo(
      const std::string& input_device_id,
      AudioSystem::OnInputDeviceInfoCallback on_input_device_info_cb);

  base::SingleThreadTaskRunner* GetTaskRunner();

 private:
  AudioParameters ComputeInputParameters(const std::string& device_id);
  AudioParameters ComputeOutputParameters(const std::string& device_id);

  AudioManager* const audio_manager_;

  DISALLOW_COPY_AND_ASSIGN(AudioSystemHelper);
};

}  // namespace media

#endif  // MEDIA_AUDIO_AUDIO_SYSTEM_HELPER_H_
