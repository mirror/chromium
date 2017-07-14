// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/audio/audio_system_info.h"

#include "media/audio/audio_manager_helper.h"

namespace audio {

namespace {

void ReplyWithOptionalAudioParams(
    base::OnceCallback<void(const base::Optional<media::AudioParameters>&)>
        reply_cb,
    const media::AudioParameters& params) {
  std::move(reply_cb).Run(params.IsValid()
                              ? base::Optional<media::AudioParameters>(params)
                              : base::Optional<media::AudioParameters>());
}

void ReplyWithDeviceDescriptions(
    AudioSystemInfo::GetDeviceDescriptionsCallback callback,
    media::AudioDeviceDescriptions descriptions) {
  std::move(callback).Run(descriptions);
}

}  // namespace

AudioSystemInfo::AudioSystemInfo(media::AudioManager* audio_manager)
    : helper_(audio_manager) {
  DCHECK(helper_.GetTaskRunner()->BelongsToCurrentThread());
}

AudioSystemInfo::~AudioSystemInfo() {
  DCHECK(helper_.GetTaskRunner()->BelongsToCurrentThread());
}

// static
std::unique_ptr<AudioSystemInfo> AudioSystemInfo::Create(
    media::AudioManager* audio_manager) {
  return base::MakeUnique<AudioSystemInfo>(audio_manager);
}

void AudioSystemInfo::Bind(
    mojom::AudioSystemInfoRequest request,
    std::unique_ptr<service_manager::ServiceContextRef> context_ref) {
  DCHECK(helper_.GetTaskRunner()->BelongsToCurrentThread());
  bindings_.AddBinding(this, std::move(request), std::move(context_ref));
}

void AudioSystemInfo::GetInputStreamParameters(
    const std::string& device_id,
    GetInputStreamParametersCallback callback) {
  DCHECK(helper_.GetTaskRunner()->BelongsToCurrentThread());
  helper_.GetInputStreamParameters(
      device_id,
      base::BindOnce(&ReplyWithOptionalAudioParams, std::move(callback)));
}

void AudioSystemInfo::GetOutputStreamParameters(
    const std::string& device_id,
    GetOutputStreamParametersCallback callback) {
  DCHECK(helper_.GetTaskRunner()->BelongsToCurrentThread());
  helper_.GetOutputStreamParameters(
      device_id,
      base::BindOnce(&ReplyWithOptionalAudioParams, std::move(callback)));
}

void AudioSystemInfo::HasInputDevices(HasInputDevicesCallback callback) {
  DCHECK(helper_.GetTaskRunner()->BelongsToCurrentThread());
  helper_.HasInputDevices(std::move(callback));
}

void AudioSystemInfo::HasOutputDevices(HasOutputDevicesCallback callback) {
  DCHECK(helper_.GetTaskRunner()->BelongsToCurrentThread());
  helper_.HasOutputDevices(std::move(callback));
}

void AudioSystemInfo::GetDeviceDescriptions(
    bool for_input,
    GetDeviceDescriptionsCallback callback) {
  DCHECK(helper_.GetTaskRunner()->BelongsToCurrentThread());
  helper_.GetDeviceDescriptions(
      for_input,
      base::BindOnce(&ReplyWithDeviceDescriptions, std::move(callback)));
}

void AudioSystemInfo::GetAssociatedOutputDeviceID(
    const std::string& input_device_id,
    GetAssociatedOutputDeviceIDCallback callback) {
  DCHECK(helper_.GetTaskRunner()->BelongsToCurrentThread());
  helper_.GetAssociatedOutputDeviceID(input_device_id, std::move(callback));
}

}  // namespace audio
