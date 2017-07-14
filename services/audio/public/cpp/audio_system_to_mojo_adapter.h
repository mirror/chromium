// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_AUDIO_PUBLIC_CPP_AUDIO_SYSTEM_TO_MOJO_ADAPTER_H_
#define SERVICES_AUDIO_PUBLIC_CPP_AUDIO_SYSTEM_TO_MOJO_ADAPTER_H_

#include <memory>

#include "base/threading/thread_checker.h"
#include "media/audio/audio_system.h"
#include "services/audio/public/interfaces/audio_system_info.mojom.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace service_manager {
class Connector;
}

namespace audio {

// NOT TESTED
//
// Creates mojom::AudioSystemInfoPtr on each request and binds it to a callback.
// So they have the same lifetime.
//
// AudioService is ref-counted. If there are no connections to it, we can tire
// it down after a timeout. So here we delete create a connect for each call
// and close it after request is completed.
//
// Another option would be to store mojom::AudioSystemInfoPtr as a member and
// monitor OnConnectionError to be able to reset/re-initialize
// mojom::AudioSystemInfoPtr if AudioService restarts. In this case we would
// open the connection of the first call and keep it open forevr (or till
// AudioService shutdown). So it does not allow to tire down AudioService if
// it's not in use.
class AudioSystemToMojoAdapter : public media::AudioSystem {
 public:
  // AudioSystemToMojoAdapter should outlive |task_runner| message loop.
  static std::unique_ptr<AudioSystem> Create(
      service_manager::Connector* connector,
      scoped_refptr<base::SingleThreadTaskRunner> task_runner);

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
  AudioSystemToMojoAdapter(
      service_manager::Connector* connector,
      scoped_refptr<base::SingleThreadTaskRunner> task_runner);

  std::unique_ptr<service_manager::Connector> connector_;

  // Task runner |connector_| is bound to / Mojo requests are issued on.
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;

  DISALLOW_COPY_AND_ASSIGN(AudioSystemToMojoAdapter);
};

}  // namespace audio

#endif  // SERVICES_AUDIO_PUBLIC_CPP_AUDIO_SYSTEM_TO_MOJO_ADAPTER_H_
