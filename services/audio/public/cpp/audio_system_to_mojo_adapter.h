// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_AUDIO_PUBLIC_CPP_AUDIO_SYSTEM_TO_MOJO_ADAPTER_H_
#define SERVICES_AUDIO_PUBLIC_CPP_AUDIO_SYSTEM_TO_MOJO_ADAPTER_H_

#include <memory>

#include "base/threading/thread_checker.h"
#include "media/audio/audio_system.h"
#include "services/audio/public/interfaces/audio_system_info.mojom.h"

namespace service_manager {
class Connector;
}

namespace audio {

class AudioSystemToMojoAdapter : public media::AudioSystem {
 public:
  using BindAudioSystemInfoCb =
      base::RepeatingCallback<void(mojom::AudioSystemInfoPtr*)>;

  static std::unique_ptr<media::AudioSystem> Create(
      service_manager::Connector* connector);

  explicit AudioSystemToMojoAdapter(
      BindAudioSystemInfoCb bind_audio_system_info_cb);

  ~AudioSystemToMojoAdapter() override;

  // AudioSystem implementation.
  void GetInputStreamParameters(const std::string& device_id,
                                OnAudioParamsCallback on_params_cb) override;

  void GetOutputStreamParameters(const std::string& device_id,
                                 OnAudioParamsCallback on_params_cb) override;

  void HasInputDevices(OnBoolCallback on_has_devices_cb) override;

  void HasOutputDevices(OnBoolCallback on_has_devices_cb) override;

  void GetDeviceDescriptions(
      bool for_input,
      OnDeviceDescriptionsCallback on_descriptions_cp) override;

  void GetAssociatedOutputDeviceID(const std::string& input_device_id,
                                   OnDeviceIdCallback on_device_id_cb) override;

  void GetInputDeviceInfo(
      const std::string& input_device_id,
      OnInputDeviceInfoCallback on_input_device_info_cb) override;

 private:
  mojom::AudioSystemInfo* GetAudioSystemInfo();
  void OnConnectionError();

  // Bound to a thread |audio_system_info_| is accessed on.
  // std::unique_ptr<service_manager::Connector> const connector_;

  BindAudioSystemInfoCb bind_audio_system_info_cb_;

  mojom::AudioSystemInfoPtr audio_system_info_;

  THREAD_CHECKER(thread_checker_);

  DISALLOW_COPY_AND_ASSIGN(AudioSystemToMojoAdapter);
};

}  // namespace audio

#endif  // SERVICES_AUDIO_PUBLIC_CPP_AUDIO_SYSTEM_TO_MOJO_ADAPTER_H_
