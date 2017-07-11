// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/audio/audio_system_impl.h"

#include <memory>
#include <string>
#include <utility>

#include "base/memory/ptr_util.h"
#include "base/single_thread_task_runner.h"
#include "base/task_runner_util.h"
#include "media/audio/audio_device_info_helper.h"
#include "media/audio/audio_manager.h"
#include "media/base/bind_to_current_loop.h"

// Using base::Unretained for |audio_manager_| is safe since it is deleted after
// its task runner, and AudioSystemImpl is deleted on the UI thread after the IO
// thread has been stopped and before |audio_manager_| deletion is scheduled.

// No need to bind callback to the current loop if we are on the audio thread.
// However, the client still expect to receive the reply asynchronously, so we
// always post the helper function which will syncronously call the (bound to
// current loop or not) callback.

namespace media {

AudioSystemImpl::AudioSystemImpl(AudioManager* audio_manager)
    : audio_manager_(audio_manager) {
  DCHECK(audio_manager_);
  AudioSystem::SetInstance(this);
}

AudioSystemImpl::~AudioSystemImpl() {
  AudioSystem::ClearInstance(this);
}

// static
std::unique_ptr<AudioSystem> AudioSystemImpl::Create(
    AudioManager* audio_manager) {
  return base::WrapUnique(new AudioSystemImpl(audio_manager));
}

void AudioSystemImpl::GetInputStreamParameters(
    const std::string& device_id,
    OnAudioParamsCallback on_params_cb) const {
  GetTaskRunner()->PostTask(
      FROM_HERE,
      base::BindOnce(&AudioDeviceInfoHelper::GetInputStreamParameters,
                     base::Unretained(audio_manager_), device_id,
                     GetTaskRunner()->BelongsToCurrentThread()
                         ? std::move(on_params_cb)
                         : media::BindToCurrentLoop(std::move(on_params_cb))));
}

void AudioSystemImpl::GetOutputStreamParameters(
    const std::string& device_id,
    OnAudioParamsCallback on_params_cb) const {
  GetTaskRunner()->PostTask(
      FROM_HERE,
      base::BindOnce(&AudioDeviceInfoHelper::GetOutputStreamParameters,
                     base::Unretained(audio_manager_), device_id,
                     GetTaskRunner()->BelongsToCurrentThread()
                         ? std::move(on_params_cb)
                         : media::BindToCurrentLoop(std::move(on_params_cb))));
}

void AudioSystemImpl::HasInputDevices(OnBoolCallback on_has_devices_cb) const {
  GetTaskRunner()->PostTask(
      FROM_HERE, base::BindOnce(&AudioDeviceInfoHelper::HasInputDevices,
                                base::Unretained(audio_manager_),
                                GetTaskRunner()->BelongsToCurrentThread()
                                    ? std::move(on_has_devices_cb)
                                    : media::BindToCurrentLoop(
                                          std::move(on_has_devices_cb))));
}

void AudioSystemImpl::HasOutputDevices(OnBoolCallback on_has_devices_cb) const {
  GetTaskRunner()->PostTask(
      FROM_HERE, base::BindOnce(&AudioDeviceInfoHelper::HasOutputDevices,
                                base::Unretained(audio_manager_),
                                GetTaskRunner()->BelongsToCurrentThread()
                                    ? std::move(on_has_devices_cb)
                                    : media::BindToCurrentLoop(
                                          std::move(on_has_devices_cb))));
}

void AudioSystemImpl::GetDeviceDescriptions(
    OnDeviceDescriptionsCallback on_descriptions_cb,
    bool for_input) {
  GetTaskRunner()->PostTask(
      FROM_HERE, base::BindOnce(&AudioDeviceInfoHelper::GetDeviceDescriptions,
                                base::Unretained(audio_manager_), for_input,
                                GetTaskRunner()->BelongsToCurrentThread()
                                    ? std::move(on_descriptions_cb)
                                    : media::BindToCurrentLoop(
                                          std::move(on_descriptions_cb))));
}

void AudioSystemImpl::GetAssociatedOutputDeviceID(
    const std::string& input_device_id,
    OnDeviceIdCallback on_device_id_cb) {
  GetTaskRunner()->PostTask(
      FROM_HERE,
      base::BindOnce(
          &AudioDeviceInfoHelper::GetAssociatedOutputDeviceID,
          base::Unretained(audio_manager_), input_device_id,
          GetTaskRunner()->BelongsToCurrentThread()
              ? std::move(on_device_id_cb)
              : media::BindToCurrentLoop(std::move(on_device_id_cb))));
}

void AudioSystemImpl::GetInputDeviceInfo(
    const std::string& input_device_id,
    OnInputDeviceInfoCallback on_input_device_info_cb) {
  GetTaskRunner()->PostTask(
      FROM_HERE,
      base::BindOnce(
          &AudioDeviceInfoHelper::GetInputDeviceInfo,
          base::Unretained(audio_manager_), input_device_id,
          GetTaskRunner()->BelongsToCurrentThread()
              ? std::move(on_input_device_info_cb)
              : media::BindToCurrentLoop(std::move(on_input_device_info_cb))));
}

base::SingleThreadTaskRunner* AudioSystemImpl::GetTaskRunner() const {
  return audio_manager_->GetTaskRunner();
}

}  // namespace media
