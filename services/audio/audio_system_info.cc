// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/audio/audio_system_info.h"

namespace audio {

AudioSystemInfo::AudioSystemInfo(media::AudioManager* audio_manager)
    : helper_(audio_manager) {
  DETACH_FROM_SEQUENCE(binding_sequence_checker_);
}

AudioSystemInfo::~AudioSystemInfo() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(binding_sequence_checker_);
}

void AudioSystemInfo::Bind(
    mojom::AudioSystemInfoRequest request,
    std::unique_ptr<service_manager::ServiceContextRef> context_ref) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(binding_sequence_checker_);
  bindings_.AddBinding(this, std::move(request), std::move(context_ref));
}

void AudioSystemInfo::GetInputStreamParameters(
    const std::string& device_id,
    GetInputStreamParametersCallback callback) {
  helper_.GetInputStreamParameters(device_id, std::move(callback));
}

void AudioSystemInfo::GetOutputStreamParameters(
    const std::string& device_id,
    GetOutputStreamParametersCallback callback) {
  helper_.GetOutputStreamParameters(device_id, std::move(callback));
}

void AudioSystemInfo::HasInputDevices(HasInputDevicesCallback callback) {
  helper_.HasInputDevices(std::move(callback));
}

void AudioSystemInfo::HasOutputDevices(HasOutputDevicesCallback callback) {
  helper_.HasOutputDevices(std::move(callback));
}

void AudioSystemInfo::GetDeviceDescriptions(
    bool for_input,
    GetDeviceDescriptionsCallback callback) {
  helper_.GetDeviceDescriptions(for_input, std::move(callback));
}

void AudioSystemInfo::GetAssociatedOutputDeviceID(
    const std::string& input_device_id,
    GetAssociatedOutputDeviceIDCallback callback) {
  helper_.GetAssociatedOutputDeviceID(input_device_id, std::move(callback));
}

void AudioSystemInfo::GetInputDeviceInfo(const std::string& input_device_id,
                                         GetInputDeviceInfoCallback callback) {
  helper_.GetInputDeviceInfo(input_device_id, std::move(callback));
}

}  // namespace audio
