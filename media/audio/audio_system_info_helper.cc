// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/audio/audio_system_info_helper.h"

#include "base/single_thread_task_runner.h"
#include "media/audio/audio_manager.h"

namespace media {

// static
void AudioSystemInfoHelper::GetInputStreamParameters(
    AudioManager* audio_manager,
    const std::string& device_id,
    OnAudioParamsCallback on_params_cb) {
  DCHECK(audio_manager);
  DCHECK(audio_manager->GetTaskRunner()->BelongsToCurrentThread());
  std::move(on_params_cb).Run(ComputeInputParameters(audio_manager, device_id));
}

// static
void AudioSystemInfoHelper::GetOutputStreamParameters(
    AudioManager* audio_manager,
    const std::string& device_id,
    OnAudioParamsCallback on_params_cb) {
  DCHECK(audio_manager);
  DCHECK(audio_manager->GetTaskRunner()->BelongsToCurrentThread());
  std::move(on_params_cb)
      .Run(ComputeOutputParameters(audio_manager, device_id));
}

// static
void AudioSystemInfoHelper::HasInputDevices(AudioManager* audio_manager,
                                            OnBoolCallback on_has_devices_cb) {
  DCHECK(audio_manager);
  DCHECK(audio_manager->GetTaskRunner()->BelongsToCurrentThread());
  std::move(on_has_devices_cb).Run(audio_manager->HasAudioInputDevices());
}

// static
void AudioSystemInfoHelper::HasOutputDevices(AudioManager* audio_manager,
                                             OnBoolCallback on_has_devices_cb) {
  DCHECK(audio_manager);
  DCHECK(audio_manager->GetTaskRunner()->BelongsToCurrentThread());
  std::move(on_has_devices_cb).Run(audio_manager->HasAudioOutputDevices());
}

// static
void AudioSystemInfoHelper::GetDeviceDescriptions(
    AudioManager* audio_manager,
    bool for_input,
    OnDeviceDescriptionsCallback on_descriptions_cb) {
  DCHECK(audio_manager);
  DCHECK(audio_manager->GetTaskRunner()->BelongsToCurrentThread());
  AudioDeviceDescriptions descriptions;
  if (for_input)
    audio_manager->GetAudioInputDeviceDescriptions(&descriptions);
  else
    audio_manager->GetAudioOutputDeviceDescriptions(&descriptions);
  std::move(on_descriptions_cb).Run(std::move(descriptions));
}

// static
void AudioSystemInfoHelper::GetAssociatedOutputDeviceID(
    AudioManager* audio_manager,
    const std::string& input_device_id,
    OnDeviceIdCallback on_device_id_cb) {
  DCHECK(audio_manager);
  DCHECK(audio_manager->GetTaskRunner()->BelongsToCurrentThread());
  std::move(on_device_id_cb)
      .Run(audio_manager->GetAssociatedOutputDeviceID(input_device_id));
}

// static
void AudioSystemInfoHelper::GetInputDeviceInfo(
    AudioManager* audio_manager,
    const std::string& input_device_id,
    OnInputDeviceInfoCallback on_input_device_info_cb) {
  DCHECK(audio_manager);
  DCHECK(audio_manager->GetTaskRunner()->BelongsToCurrentThread());
  const std::string associated_output_device_id =
      audio_manager->GetAssociatedOutputDeviceID(input_device_id);

  std::move(on_input_device_info_cb)
      .Run(ComputeInputParameters(audio_manager, input_device_id),
           associated_output_device_id.empty()
               ? AudioParameters()
               : ComputeOutputParameters(audio_manager,
                                         associated_output_device_id),
           associated_output_device_id);
}

// static
AudioParameters AudioSystemInfoHelper::ComputeInputParameters(
    AudioManager* audio_manager,
    const std::string& device_id) {
  DCHECK(audio_manager);
  DCHECK(audio_manager->GetTaskRunner()->BelongsToCurrentThread());

  // TODO(olka): remove this when AudioManager::GetInputStreamParameters()
  // returns invalid parameters if the device is not found.
  if (AudioDeviceDescription::IsLoopbackDevice(device_id)) {
    // For system audio capture, we need an output device (namely speaker)
    // instead of an input device (namely microphone) to work.
    // AudioManager::GetInputStreamParameters will check |device_id| and
    // query the correct device for audio parameters by itself.
    if (!audio_manager->HasAudioOutputDevices())
      return AudioParameters();
  } else {
    if (!audio_manager->HasAudioInputDevices())
      return AudioParameters();
  }
  return audio_manager->GetInputStreamParameters(device_id);
}

// static
AudioParameters AudioSystemInfoHelper::ComputeOutputParameters(
    AudioManager* audio_manager,
    const std::string& device_id) {
  DCHECK(audio_manager);
  DCHECK(audio_manager->GetTaskRunner()->BelongsToCurrentThread());

  // TODO(olka): remove this when
  // AudioManager::Get[Default]OutputStreamParameters() returns invalid
  // parameters if the device is not found.
  if (!audio_manager->HasAudioOutputDevices())
    return AudioParameters();

  return media::AudioDeviceDescription::IsDefaultDevice(device_id)
             ? audio_manager->GetDefaultOutputStreamParameters()
             : audio_manager->GetOutputStreamParameters(device_id);
}

}  // namespace media
