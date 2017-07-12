// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_AUDIO_AUDIO_SYSTEM_INFO_HELPER_H_
#define MEDIA_AUDIO_AUDIO_SYSTEM_INFO_HELPER_H_

#include <string>

#include "base/callback.h"
#include "media/audio/audio_device_description.h"
#include "media/base/audio_parameters.h"
#include "media/base/media_export.h"

namespace media {
class AudioManager;

// A set of helper functions built on top of AudioManager; only AudioSystem
// implementations are allowed to access them.
// To be called on audio thread only.
class MEDIA_EXPORT AudioSystemInfoHelper {
 private:
  friend class AudioSystemImpl;

  using OnAudioParamsCallback =
      base::OnceCallback<void(const AudioParameters&)>;
  using OnBoolCallback = base::OnceCallback<void(bool)>;
  using OnDeviceDescriptionsCallback =
      base::OnceCallback<void(AudioDeviceDescriptions)>;
  using OnDeviceIdCallback = base::OnceCallback<void(const std::string&)>;
  using OnInputDeviceInfoCallback = base::OnceCallback<
      void(const AudioParameters&, const AudioParameters&, const std::string&)>;

  // Callback may receive invalid parameters, it means the specified device is
  // not found. This is best-effort: valid parameters do not guarantee existence
  // of the device.
  static void GetInputStreamParameters(AudioManager* audio_manager,
                                       const std::string& device_id,
                                       OnAudioParamsCallback on_params_cb);

  // If media::AudioDeviceDescription::IsDefaultDevice(device_id) is true,
  // callback will receive the parameters of the default output device.
  // Callback may receive invalid parameters, it means the specified device is
  // not found. This is best-effort: valid parameters do not guarantee existence
  // of the device.
  // TODO(olka,tommi): fix all AudioManager implementations to return invalid
  // parameters if the device is not found.
  static void GetOutputStreamParameters(AudioManager* audio_manager,
                                        const std::string& device_id,
                                        OnAudioParamsCallback on_params_cb);

  static void HasInputDevices(AudioManager* audio_manager,
                              OnBoolCallback on_has_devices_cb);

  static void HasOutputDevices(AudioManager* audio_manager,
                               OnBoolCallback on_has_devices_cb);

  // Replies with device descriptions of input audio devices if |for_input| is
  // true, and of output audio devices otherwise.
  static void GetDeviceDescriptions(
      AudioManager* audio_manager,
      bool for_input,
      OnDeviceDescriptionsCallback on_descriptions_cb);

  // Replies with an empty string if there is no associated output device found.
  static void GetAssociatedOutputDeviceID(AudioManager* audio_manager,
                                          const std::string& input_device_id,
                                          OnDeviceIdCallback on_device_id_cb);

  // Replies with audio parameters for the specified input device and audio
  // parameters and device ID of the associated output device, if any (otherwise
  // it's AudioParameters() and an empty string).
  static void GetInputDeviceInfo(
      AudioManager* audio_manager,
      const std::string& input_device_id,
      OnInputDeviceInfoCallback on_input_device_info_cb);

  static AudioParameters ComputeInputParameters(AudioManager* audio_manager,
                                                const std::string& device_id);

  static AudioParameters ComputeOutputParameters(AudioManager* audio_manager,
                                                 const std::string& device_id);
};

}  // namespace media

#endif  // MEDIA_AUDIO_AUDIO_SYSTEM_INFO_HELPER_H_
