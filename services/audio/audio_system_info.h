// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_AUDIO_AUDIO_SYSTEM_INFO_H_
#define SERVICES_AUDIO_AUDIO_SYSTEM_INFO_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/sequence_checker.h"
#include "media/audio/audio_system_helper.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "services/audio/public/interfaces/audio_system_info.mojom.h"
#include "services/service_manager/public/cpp/service_context_ref.h"

namespace media {
class AudioManager;
}

namespace audio {

class AudioSystemInfo : public mojom::AudioSystemInfo {
 public:
  explicit AudioSystemInfo(media::AudioManager* audio_manager);
  ~AudioSystemInfo() override;

  void Bind(mojom::AudioSystemInfoRequest request,
            std::unique_ptr<service_manager::ServiceContextRef>);

 private:
  // audio::mojom::AudioSystemInfo implementation.
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

  void GetInputDeviceInfo(const std::string& input_device_id,
                          GetInputDeviceInfoCallback callback) override;

  media::AudioSystemHelper helper_;

  // Each binding increases ref count of the service context, so that the
  // service knows when it is in use.
  mojo::BindingSet<mojom::AudioSystemInfo,
                   std::unique_ptr<service_manager::ServiceContextRef>>
      bindings_;

  // Validates thread-safe access to |bindings_| only. |helper_| takes care of
  // its thread safity/affinity itself.
  SEQUENCE_CHECKER(binding_sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(AudioSystemInfo);
};

}  // namespace audio

#endif  // SERVICES_AUDIO_AUDIO_SYSTEM_INFO_H_
