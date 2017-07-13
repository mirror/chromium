// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/audio/audio_system_impl.h"

#include "base/memory/ptr_util.h"
#include "base/single_thread_task_runner.h"
#include "base/task_runner_util.h"
#include "media/base/bind_to_current_loop.h"

// Using base::Unretained for |non_thread_safe_| is safe since it is deleted
// after its task runner, and AudioSystemImpl is deleted on the UI thread after
// the IO thread has been stopped and before |non_thread_safe_| deletion is
// scheduled.

// No need to bind callback to the current loop if we are on the audio thread.
// However, the client still expect to receive the reply asynchronously, so we
// always post the helper function which will syncronously call the (bound to
// current loop or not) callback.

namespace media {

template <typename... Args>
inline base::OnceCallback<void(Args...)>
AudioSystemImpl::MaybeBindToCurrentLoop(
    base::OnceCallback<void(Args...)> callback) {
  return task_runner()->BelongsToCurrentThread()
             ? std::move(callback)
             : media::BindToCurrentLoop(std::move(callback));
}

AudioSystemImpl::AudioSystemImpl(
    std::unique_ptr<AudioSystemNonThreadSafe> non_thread_safe)
    : non_thread_safe_(std::move(non_thread_safe)) {
  AudioSystem::SetInstance(this);
}

AudioSystemImpl::~AudioSystemImpl() {
  AudioSystem::ClearInstance(this);
}

void AudioSystemImpl::GetInputStreamParameters(
    const std::string& device_id,
    OnAudioParamsCallback on_params_cb) {
  task_runner()->PostTask(
      FROM_HERE,
      base::BindOnce(&AudioSystemNonThreadSafe::GetInputStreamParameters,
                     base::Unretained(non_thread_safe_.get()), device_id,
                     MaybeBindToCurrentLoop(std::move(on_params_cb))));
}

void AudioSystemImpl::GetOutputStreamParameters(
    const std::string& device_id,
    OnAudioParamsCallback on_params_cb) {
  task_runner()->PostTask(
      FROM_HERE,
      base::BindOnce(&AudioSystemNonThreadSafe::GetOutputStreamParameters,
                     base::Unretained(non_thread_safe_.get()), device_id,
                     MaybeBindToCurrentLoop(std::move(on_params_cb))));
}

void AudioSystemImpl::HasInputDevices(OnBoolCallback on_has_devices_cb) {
  task_runner()->PostTask(
      FROM_HERE,
      base::BindOnce(&AudioSystemNonThreadSafe::HasInputDevices,
                     base::Unretained(non_thread_safe_.get()),
                     MaybeBindToCurrentLoop(std::move(on_has_devices_cb))));
}

void AudioSystemImpl::HasOutputDevices(OnBoolCallback on_has_devices_cb) {
  task_runner()->PostTask(
      FROM_HERE,
      base::BindOnce(&AudioSystemNonThreadSafe::HasOutputDevices,
                     base::Unretained(non_thread_safe_.get()),
                     MaybeBindToCurrentLoop(std::move(on_has_devices_cb))));
}

void AudioSystemImpl::GetDeviceDescriptions(
    bool for_input,
    OnDeviceDescriptionsCallback on_descriptions_cb) {
  task_runner()->PostTask(
      FROM_HERE,
      base::BindOnce(&AudioSystemNonThreadSafe::GetDeviceDescriptions,
                     base::Unretained(non_thread_safe_.get()), for_input,
                     MaybeBindToCurrentLoop(std::move(on_descriptions_cb))));
}

void AudioSystemImpl::GetAssociatedOutputDeviceID(
    const std::string& input_device_id,
    OnDeviceIdCallback on_device_id_cb) {
  task_runner()->PostTask(
      FROM_HERE,
      base::BindOnce(&AudioSystemNonThreadSafe::GetAssociatedOutputDeviceID,
                     base::Unretained(non_thread_safe_.get()), input_device_id,
                     MaybeBindToCurrentLoop(std::move(on_device_id_cb))));
}

void AudioSystemImpl::GetInputDeviceInfo(
    const std::string& input_device_id,
    OnInputDeviceInfoCallback on_input_device_info_cb) {
  task_runner()->PostTask(
      FROM_HERE,
      base::BindOnce(
          &AudioSystemNonThreadSafe::GetInputDeviceInfo,
          base::Unretained(non_thread_safe_.get()), input_device_id,
          MaybeBindToCurrentLoop(std::move(on_input_device_info_cb))));
}

}  // namespace media
