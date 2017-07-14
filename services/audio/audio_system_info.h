// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_AUDIO_AUDIO_SYSTEM_INFO_H_
#define SERVICES_AUDIO_AUDIO_SYSTEM_INFO_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "media/audio/audio_manager_helper.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "services/audio/public/interfaces/audio_system_info.mojom.h"
#include "services/service_manager/public/cpp/service_context_ref.h"

namespace media {
class AudioManager;
}

namespace audio {

class AudioSystemInfo : public mojom::AudioSystemInfo {
 public:
  AudioSystemInfo(media::AudioManager* audio_manager);
  ~AudioSystemInfo() override;

  static std::unique_ptr<AudioSystemInfo> Create(
      media::AudioManager* audio_manager);

  void Bind(mojom::AudioSystemInfoRequest request,
            std::unique_ptr<service_manager::ServiceContextRef>);

 private:
  void GetInputStreamParameters(
      const std::string& device_id,
      GetInputStreamParametersCallback callback) override;
  void GetOutputStreamParameters(
      const std::string& device_id,
      GetOutputStreamParametersCallback callback) override;
  void HasInputDevices(HasInputDevicesCallback callback) override;
  void HasOutputDevices(HasOutputDevicesCallback callback) override;
  void GetDeviceDescriptions(bool for_input,
                             GetDeviceDescriptionsCallback callback) override;
  void GetAssociatedOutputDeviceID(
      const std::string& input_device_id,
      GetAssociatedOutputDeviceIDCallback callback) override;

  media::AudioManagerHelper helper_;

  // Each binding increases ref count of the service context, so that the
  // service knows when it is in use.
  mojo::BindingSet<mojom::AudioSystemInfo,
                   std::unique_ptr<service_manager::ServiceContextRef>>
      bindings_;

  DISALLOW_COPY_AND_ASSIGN(AudioSystemInfo);
};

}  // namespace audio

#endif  // SERVICES_AUDIO_AUDIO_SYSTEM_INFO_H_
