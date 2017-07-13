// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_AUDIO_LOCAL_AUDIO_SYSTEM_H_
#define MEDIA_AUDIO_LOCAL_AUDIO_SYSTEM_H_

#include "media/audio/audio_system.h"
#include "media/base/media_export.h"

namespace media {
class AudioManager;

class MEDIA_EXPORT LocalAudioSystem : public AudioSystemNonThreadSafe {
 public:
  LocalAudioSystem(AudioManager* audio_manager);
  ~LocalAudioSystem() override;

  // AudioSystem implementation.
  void GetInputStreamParameters(const std::string& device_id,
                                OnAudioParamsCallback on_params_cb) override;

  void GetOutputStreamParameters(const std::string& device_id,
                                 OnAudioParamsCallback on_params_cb) override;

  void HasInputDevices(OnBoolCallback on_has_devices_cb) override;

  void HasOutputDevices(OnBoolCallback on_has_devices_cb) override;

  void GetDeviceDescriptions(
      bool for_input,
      OnDeviceDescriptionsCallback on_descriptions_cp) override;

  void GetAssociatedOutputDeviceID(const std::string& input_device_id,
                                   OnDeviceIdCallback on_device_id_cb) override;

  void GetInputDeviceInfo(
      const std::string& input_device_id,
      OnInputDeviceInfoCallback on_input_device_info_cb) override;

  // AudioSystemNonThreadSafe implementation.
  base::SingleThreadTaskRunner* GetTaskRunner() override;

 private:
  AudioParameters ComputeInputParameters(const std::string& device_id);
  AudioParameters ComputeOutputParameters(const std::string& device_id);

  AudioManager* const audio_manager_;
};

}  // namespace media

#endif  // MEDIA_AUDIO_LOCAL_AUDIO_SYSTEM_H_
