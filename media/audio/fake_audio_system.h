// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_AUDIO_FAKE_AUDIO_SYSTEM_H_
#define MEDIA_AUDIO_FAKE_AUDIO_SYSTEM_H_

#include <string>

#include "base/sequence_checker.h"
#include "media/audio/audio_system.h"
#include "media/base/media_export.h"

namespace media {

class MEDIA_EXPORT FakeAudioSystem : public AudioSystem {
 public:
  FakeAudioSystem();

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
  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(FakeAudioSystem);
};

}  // namespace media

#endif  // MEDIA_AUDIO_FAKE_AUDIO_SYSTEM_H_
