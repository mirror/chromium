// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/audio/public/cpp/audio_system_to_mojo_adapter.h"

#include "base/bind.h"
#include "media/base/scoped_callback_runner.h"
#include "services/audio/public/interfaces/constants.mojom.h"
#include "services/service_manager/public/cpp/connector.h"

namespace audio {

namespace {

base::Optional<media::AudioParameters> EmptyParams() {
  return base::Optional<media::AudioParameters>();
}

}  // namespace

AudioSystemToMojoAdapter::AudioSystemToMojoAdapter(
    service_manager::Connector* connector)
    : connector_(connector ? connector->Clone() : nullptr) {
  DCHECK(connector_);
  DETACH_FROM_THREAD(thread_checker_);
}

AudioSystemToMojoAdapter::~AudioSystemToMojoAdapter() {}

void AudioSystemToMojoAdapter::GetInputStreamParameters(
    const std::string& device_id,
    OnAudioParamsCallback on_params_cb) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  mojom::AudioSystemInfo* audio_system_info = GetAudioSystemInfo();
  if (!audio_system_info) {
    std::move(on_params_cb).Run(EmptyParams());
    return;
  }
  audio_system_info->GetInputStreamParameters(
      device_id,
      media::ScopedCallbackRunner(std::move(on_params_cb), EmptyParams()));
}

void AudioSystemToMojoAdapter::GetOutputStreamParameters(
    const std::string& device_id,
    OnAudioParamsCallback on_params_cb) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  mojom::AudioSystemInfo* audio_system_info = GetAudioSystemInfo();
  if (!audio_system_info) {
    std::move(on_params_cb).Run(EmptyParams());
    return;
  }
  audio_system_info->GetOutputStreamParameters(
      device_id,
      media::ScopedCallbackRunner(std::move(on_params_cb), EmptyParams()));
}

void AudioSystemToMojoAdapter::HasInputDevices(
    OnBoolCallback on_has_devices_cb) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  mojom::AudioSystemInfo* audio_system_info = GetAudioSystemInfo();
  if (!audio_system_info) {
    std::move(on_has_devices_cb).Run(false);
    return;
  }
  audio_system_info->HasInputDevices(
      media::ScopedCallbackRunner(std::move(on_has_devices_cb), false));
}

void AudioSystemToMojoAdapter::HasOutputDevices(
    OnBoolCallback on_has_devices_cb) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  mojom::AudioSystemInfo* audio_system_info = GetAudioSystemInfo();
  if (!audio_system_info) {
    std::move(on_has_devices_cb).Run(false);
    return;
  }
  audio_system_info->HasOutputDevices(
      media::ScopedCallbackRunner(std::move(on_has_devices_cb), false));
}

void AudioSystemToMojoAdapter::GetDeviceDescriptions(
    bool for_input,
    OnDeviceDescriptionsCallback on_descriptions_cb) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  mojom::AudioSystemInfo* audio_system_info = GetAudioSystemInfo();
  if (!audio_system_info) {
    std::move(on_descriptions_cb).Run(media::AudioDeviceDescriptions());
    return;
  }
  audio_system_info->GetDeviceDescriptions(
      for_input, media::ScopedCallbackRunner(std::move(on_descriptions_cb),
                                             media::AudioDeviceDescriptions()));
}

void AudioSystemToMojoAdapter::GetAssociatedOutputDeviceID(
    const std::string& input_device_id,
    OnDeviceIdCallback on_device_id_cb) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  mojom::AudioSystemInfo* audio_system_info = GetAudioSystemInfo();
  if (!audio_system_info) {
    std::move(on_device_id_cb).Run(std::string());
    return;
  }
  audio_system_info->GetAssociatedOutputDeviceID(
      input_device_id,
      media::ScopedCallbackRunner(std::move(on_device_id_cb), std::string()));
}

void AudioSystemToMojoAdapter::GetInputDeviceInfo(
    const std::string& input_device_id,
    OnInputDeviceInfoCallback on_input_device_info_cb) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  mojom::AudioSystemInfo* audio_system_info = GetAudioSystemInfo();
  if (!audio_system_info) {
    std::move(on_input_device_info_cb)
        .Run(EmptyParams(), EmptyParams(), std::string());
    return;
  }
  audio_system_info->GetInputDeviceInfo(
      input_device_id,
      media::ScopedCallbackRunner(std::move(on_input_device_info_cb),
                                  EmptyParams(), EmptyParams(), std::string()));
}

mojom::AudioSystemInfo* AudioSystemToMojoAdapter::GetAudioSystemInfo() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (!audio_system_info_) {
    connector_->BindInterface(mojom::kServiceName, &audio_system_info_);
    audio_system_info_.set_connection_error_handler(base::Bind(
        &AudioSystemToMojoAdapter::OnConnectionError, base::Unretained(this)));
  }
  return audio_system_info_.get();
}

void AudioSystemToMojoAdapter::OnConnectionError() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  audio_system_info_.reset();
}

}  // namespace audio
