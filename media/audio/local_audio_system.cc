// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/audio/local_audio_system.h"

#include "base/single_thread_task_runner.h"
#include "media/audio/audio_manager.h"

namespace media {

LocalAudioSystem::LocalAudioSystem(AudioManager* audio_manager)
    : audio_manager_(audio_manager) {
  DCHECK(audio_manager_);
}

LocalAudioSystem::~LocalAudioSystem() {}

void LocalAudioSystem::GetInputStreamParameters(
    const std::string& device_id,
    OnAudioParamsCallback on_params_cb) {
  DCHECK(GetTaskRunner()->BelongsToCurrentThread());
  std::move(on_params_cb).Run(ComputeInputParameters(device_id));
}

void LocalAudioSystem::GetOutputStreamParameters(
    const std::string& device_id,
    OnAudioParamsCallback on_params_cb) {
  DCHECK(GetTaskRunner()->BelongsToCurrentThread());
  std::move(on_params_cb).Run(ComputeOutputParameters(device_id));
}

void LocalAudioSystem::HasInputDevices(OnBoolCallback on_has_devices_cb) {
  DCHECK(GetTaskRunner()->BelongsToCurrentThread());
  std::move(on_has_devices_cb).Run(audio_manager_->HasAudioInputDevices());
}

void LocalAudioSystem::HasOutputDevices(OnBoolCallback on_has_devices_cb) {
  DCHECK(GetTaskRunner()->BelongsToCurrentThread());
  std::move(on_has_devices_cb).Run(audio_manager_->HasAudioOutputDevices());
}

void LocalAudioSystem::GetDeviceDescriptions(
    bool for_input,
    OnDeviceDescriptionsCallback on_descriptions_cb) {
  DCHECK(GetTaskRunner()->BelongsToCurrentThread());
  AudioDeviceDescriptions descriptions;
  if (for_input)
    audio_manager_->GetAudioInputDeviceDescriptions(&descriptions);
  else
    audio_manager_->GetAudioOutputDeviceDescriptions(&descriptions);
  std::move(on_descriptions_cb).Run(std::move(descriptions));
}

void LocalAudioSystem::GetAssociatedOutputDeviceID(
    const std::string& input_device_id,
    OnDeviceIdCallback on_device_id_cb) {
  DCHECK(GetTaskRunner()->BelongsToCurrentThread());
  std::move(on_device_id_cb)
      .Run(audio_manager_->GetAssociatedOutputDeviceID(input_device_id));
}

void LocalAudioSystem::GetInputDeviceInfo(
    const std::string& input_device_id,
    OnInputDeviceInfoCallback on_input_device_info_cb) {
  DCHECK(GetTaskRunner()->BelongsToCurrentThread());
  const std::string associated_output_device_id =
      audio_manager_->GetAssociatedOutputDeviceID(input_device_id);

  std::move(on_input_device_info_cb)
      .Run(ComputeInputParameters(input_device_id),
           associated_output_device_id.empty()
               ? AudioParameters()
               : ComputeOutputParameters(associated_output_device_id),
           associated_output_device_id);
}

base::SingleThreadTaskRunner* LocalAudioSystem::GetTaskRunner() {
  return audio_manager_->GetTaskRunner();
}

AudioParameters LocalAudioSystem::ComputeInputParameters(
    const std::string& device_id) {
  DCHECK(GetTaskRunner()->BelongsToCurrentThread());

  // TODO(olka): remove this when AudioManager::GetInputStreamParameters()
  // returns invalid parameters if the device is not found.
  if (AudioDeviceDescription::IsLoopbackDevice(device_id)) {
    // For system audio capture, we need an output device (namely speaker)
    // instead of an input device (namely microphone) to work.
    // AudioManager::GetInputStreamParameters will check |device_id| and
    // query the correct device for audio parameters by itself.
    if (!audio_manager_->HasAudioOutputDevices())
      return AudioParameters();
  } else {
    if (!audio_manager_->HasAudioInputDevices())
      return AudioParameters();
  }
  return audio_manager_->GetInputStreamParameters(device_id);
}

AudioParameters LocalAudioSystem::ComputeOutputParameters(
    const std::string& device_id) {
  DCHECK(GetTaskRunner()->BelongsToCurrentThread());

  // TODO(olka): remove this when
  // AudioManager::Get[Default]OutputStreamParameters() returns invalid
  // parameters if the device is not found.
  if (!audio_manager_->HasAudioOutputDevices())
    return AudioParameters();

  return media::AudioDeviceDescription::IsDefaultDevice(device_id)
             ? audio_manager_->GetDefaultOutputStreamParameters()
             : audio_manager_->GetOutputStreamParameters(device_id);
}

}  // namespace media
