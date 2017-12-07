// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/audio/public/cpp/audio_system_to_mojo_adapter.h"

#include "base/bind.h"
#include "media/base/scoped_callback_runner.h"
#include "services/audio/public/interfaces/constants.mojom.h"
#include "services/service_manager/public/cpp/connector.h"

namespace audio {

// static
std::unique_ptr<media::AudioSystem> AudioSystemToMojoAdapter::Create(
    service_manager::Connector* connector) {
  DCHECK(connector);

  // BindSystemInfoCb callback will own this copy of the connector, and it
  // will be bound to the thread AudioSystemToMojoAdapter is used on.
  std::unique_ptr<service_manager::Connector> owned_connector =
      connector->Clone();

  // Disambiguating overloaded template method
  // service_manager::Connector::BindInterface() and binding connector and
  // service name to it.
  BindSystemInfoCb cb = base::BindRepeating(
      static_cast<void (service_manager::Connector::*)(
          const std::string&, mojom::SystemInfoRequest)>(
          &service_manager::Connector::BindInterface<mojom::SystemInfo>),
      std::move(owned_connector), mojom::kServiceName);

  return std::make_unique<AudioSystemToMojoAdapter>(std::move(cb));
}

AudioSystemToMojoAdapter::AudioSystemToMojoAdapter(
    BindSystemInfoCb bind_system_info_cb)
    : bind_system_info_cb_(std::move(bind_system_info_cb)) {
  DCHECK(!bind_system_info_cb_.is_null());
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
  if (!system_info_)
    bind_system_info_cb_.Run(mojo::MakeRequest(&system_info_));
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
