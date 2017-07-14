// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/audio/public/cpp/audio_system_to_mojo_adapter.h"

#include "base/bind.h"
#include "base/single_thread_task_runner.h"
#include "media/base/bind_to_current_loop.h"
#include "media/base/scoped_callback_runner.h"
#include "services/audio/public/interfaces/constants.mojom.h"
#include "services/service_manager/public/cpp/connector.h"

namespace audio {

namespace {

// TODO: Too much binding.

// TODO: fix templates.
template <typename... Args>
void InterfacePtrOwner(mojom::AudioSystemInfoPtr interface_ptr,
                       base::OnceCallback<void(Args...)> callback,
                       Args... args) {
  std::move(callback).Run(std::forward<Args>(args)...);
  // |interface_ptr| will be destroyed now.
}

// Contrstucts a one-parameter callback which owns InterfacePtr it will be
// passed to and which will be called with |on_error_arg| on connection error or
// on InterfacePtr message loop destruction.
// TODO: this means cliens of AudioSystem should make sure callbacks are valid
// after |task_runner_| message loop destruction, or the loop AudioSystem is
// called on is shut down before |task_runner_| message loop.
template <typename T>
base::OnceCallback<void(T)> BindPerfectReplyCb(
    // InterfacePtr the reply callback is passed to.
    mojom::AudioSystemInfoPtr interface_ptr,
    // Reply callback.
    base::OnceCallback<void(T)> callback,
    // Default arguments to pass to run |callback| with if it was not called by
    // Mojo (in case of connection error (or message loop destruction???)
    T&& on_error_arg) {
  return base::BindOnce(
      &InterfacePtrOwner<T>, std::move(interface_ptr),
      media::ScopedCallbackRunner(std::move(callback),
                                  std::forward<T>(on_error_arg)));
}

// Converts empty Otional parameters to invalid ones.
// TODO: make AudioSystem to always return valid parameters? (See speech
// recognition, it checks parameters valididty now)
/*
void MaybeRunWithInvalidParams(media::AudioSystem::OnAudioParamsCallback cb,
base::Optional<media::AudioParameters> params) { std::move(cb).Run(params?
params.value() : media::AudioParameters());
}
*/
}  // namespace

AudioSystemToMojoAdapter::AudioSystemToMojoAdapter(
    service_manager::Connector* connector,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner)
    : connector_(connector->Clone()), task_runner_(std::move(task_runner)) {
  DCHECK(connector_);
  AudioSystem::SetInstance(this);
}

AudioSystemToMojoAdapter::~AudioSystemToMojoAdapter() {
  AudioSystem::ClearInstance(this);
}

// static
std::unique_ptr<media::AudioSystem> AudioSystemToMojoAdapter::Create(
    service_manager::Connector* connector,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner) {
  return base::WrapUnique(
      new AudioSystemToMojoAdapter(connector, std::move(task_runner)));
}

// AudioSystem implementation.
void AudioSystemToMojoAdapter::GetInputStreamParameters(
    const std::string& device_id,
    OnAudioParamsCallback on_params_cb) {
  // Fix templates
  /*
    if (!task_runner_->BelongsToCurrentThread()) {
      // base::Unretained(this) is safe since |this| oultives |task_runner_|
      // message loop.
      task_runner_->PostTask(
          FROM_HERE,
          base::BindOnce(&AudioSystemToMojoAdapter::GetInputStreamParameters,
                         base::Unretained(this), device_id,
                         // Reply will have an extra hop from |task_runner_| to
    the current message loop.
                         media::BindToCurrentLoop(std::move(on_params_cb))));
      return;
    }

    mojom::AudioSystemInfoPtr audio_system_info;
    connector_->BindInterface(audio::mojom::kServiceName, &audio_system_info);
    audio_system_info->GetInputStreamParameters(device_id, BindPerfectReplyCb(
        std::move(audio_system_info), base::BindOnce(&MaybeRunWithInvalidParams,
    std::move(on_params_cb)), base::Optional<media::AudioParameters>())); //
    TODO: return UnavailableParams?
  */
}

void AudioSystemToMojoAdapter::GetOutputStreamParameters(
    const std::string& device_id,
    OnAudioParamsCallback on_params_cb) {
  // TODO
}

void AudioSystemToMojoAdapter::HasInputDevices(
    OnBoolCallback on_has_devices_cb) {
  if (!task_runner_->BelongsToCurrentThread()) {
    // base::Unretained(this) is safe since |this| oultives |task_runner_|
    // message loop.
    task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(&AudioSystemToMojoAdapter::HasInputDevices,
                       base::Unretained(this),
                       // Reply will have an extra hop from |task_runner_| to
                       // the current message loop.
                       media::BindToCurrentLoop(std::move(on_has_devices_cb))));
    return;
  }

  mojom::AudioSystemInfoPtr audio_system_info;
  connector_->BindInterface(audio::mojom::kServiceName, &audio_system_info);
  audio_system_info->HasInputDevices(BindPerfectReplyCb(
      std::move(audio_system_info), std::move(on_has_devices_cb), false));
}

void AudioSystemToMojoAdapter::HasOutputDevices(
    OnBoolCallback on_has_devices_cb) {
  // TODO
}

void AudioSystemToMojoAdapter::GetDeviceDescriptions(
    bool for_input,
    OnDeviceDescriptionsCallback on_descriptions_cb) {
  // Fix templates
  /*
    if (!task_runner_->BelongsToCurrentThread()) {
      // base::Unretained(this) is safe since |this| oultives |task_runner_|
      // message loop.
      task_runner_->PostTask(
          FROM_HERE,
          base::BindOnce(&AudioSystemToMojoAdapter::GetDeviceDescriptions,
                         base::Unretained(this), for_input,
                         // Reply will have an extra hop from |task_runner_|to
                         // the current message loop.
                         media::BindToCurrentLoop(
                           std::move(on_descriptions_cb))));
      return;
    }

    mojom::AudioSystemInfoPtr audio_system_info;
    connector_->BindInterface(audio::mojom::kServiceName, &audio_system_info);
    audio_system_info->GetDeviceDescriptions(for_input, BindPerfectReplyCb(
        std::move(audio_system_info), std::move(on_descriptions_cb),
    media::AudioDeviceDescriptions()));
  */
}

void AudioSystemToMojoAdapter::GetAssociatedOutputDeviceID(
    const std::string& input_device_id,
    OnDeviceIdCallback on_device_id_cb) {
  // Fix templates
  /*
    if (!task_runner_->BelongsToCurrentThread()) {
      // base::Unretained(this) is safe since |this| oultives |task_runner_|
      // message loop.
      task_runner_->PostTask(
          FROM_HERE,
          base::BindOnce(&AudioSystemToMojoAdapter::GetAssociatedOutputDeviceID,
                         base::Unretained(this), input_device_id,
                         // Reply will have an extra hop from |task_runner_| to
    the current message loop.
                         media::BindToCurrentLoop(std::move(on_device_id_cb))));
      return;
    }

    mojom::AudioSystemInfoPtr audio_system_info;
    connector_->BindInterface(audio::mojom::kServiceName, &audio_system_info);
    audio_system_info->GetAssociatedOutputDeviceID(input_device_id,
    BindPerfectReplyCb( std::move(audio_system_info),
    std::move(on_device_id_cb), std::string()));
  */
}

void AudioSystemToMojoAdapter::GetInputDeviceInfo(
    const std::string& input_device_id,
    OnInputDeviceInfoCallback on_input_device_info_cb) {
  // TODO: sequentially get input params, associated output device id,
  // associated output device params; OR add another Mojo method (will work
  // faster!).
}

}  // namespace audio
