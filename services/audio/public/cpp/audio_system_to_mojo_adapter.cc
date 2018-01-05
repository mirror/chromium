// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/audio/public/cpp/audio_system_to_mojo_adapter.h"

#include "base/bind.h"
#include "media/base/scoped_callback_runner.h"
#include "services/audio/public/interfaces/constants.mojom.h"
#include "services/service_manager/public/cpp/connector.h"

namespace audio {

AudioSystemToMojoAdapter::AudioSystemToMojoAdapter(
    std::unique_ptr<service_manager::Connector> connector)
    : connector_(std::move(connector)) {
  DCHECK(connector_);
  DETACH_FROM_THREAD(thread_checker_);
}

AudioSystemToMojoAdapter::~AudioSystemToMojoAdapter() {}

void AudioSystemToMojoAdapter::GetInputStreamParameters(
    const std::string& device_id,
    OnAudioParamsCallback on_params_cb) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  GetSystemInfo()->GetInputStreamParameters(
      device_id,
      media::ScopedCallbackRunner(std::move(on_params_cb), base::nullopt));
}

void AudioSystemToMojoAdapter::GetOutputStreamParameters(
    const std::string& device_id,
    OnAudioParamsCallback on_params_cb) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  GetSystemInfo()->GetOutputStreamParameters(
      device_id,
      media::ScopedCallbackRunner(std::move(on_params_cb), base::nullopt));
}

void AudioSystemToMojoAdapter::HasInputDevices(
    OnBoolCallback on_has_devices_cb) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  GetSystemInfo()->HasInputDevices(
      media::ScopedCallbackRunner(std::move(on_has_devices_cb), false));
}

void AudioSystemToMojoAdapter::HasOutputDevices(
    OnBoolCallback on_has_devices_cb) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  GetSystemInfo()->HasOutputDevices(
      media::ScopedCallbackRunner(std::move(on_has_devices_cb), false));
}

void AudioSystemToMojoAdapter::GetDeviceDescriptions(
    bool for_input,
    OnDeviceDescriptionsCallback on_descriptions_cb) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  GetSystemInfo()->GetDeviceDescriptions(
      for_input, media::ScopedCallbackRunner(std::move(on_descriptions_cb),
                                             media::AudioDeviceDescriptions()));
}

void AudioSystemToMojoAdapter::GetAssociatedOutputDeviceID(
    const std::string& input_device_id,
    OnDeviceIdCallback on_device_id_cb) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  GetSystemInfo()->GetAssociatedOutputDeviceID(
      input_device_id,
      media::ScopedCallbackRunner(std::move(on_device_id_cb), std::string()));
}

void AudioSystemToMojoAdapter::GetInputDeviceInfo(
    const std::string& input_device_id,
    OnInputDeviceInfoCallback on_input_device_info_cb) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  GetSystemInfo()->GetInputDeviceInfo(
      input_device_id,
      media::ScopedCallbackRunner(std::move(on_input_device_info_cb),
                                  base::nullopt, base::nullopt, std::string()));
}

mojom::SystemInfo* AudioSystemToMojoAdapter::GetSystemInfo() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (!system_info_) {
    connector_->BindInterface(mojom::kServiceName,
                              mojo::MakeRequest(&system_info_));
  }
  DCHECK(system_info_);
  system_info_.set_connection_error_handler(base::BindOnce(
      &AudioSystemToMojoAdapter::OnConnectionError, base::Unretained(this)));
  return system_info_.get();
}

void AudioSystemToMojoAdapter::OnConnectionError() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  system_info_.reset();
}

}  // namespace audio
