// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media_recorder/audio_track_pcm_encoder.h"

#include <string.h>
#include <string>
#include <vector>

#include "base/sys_byteorder.h"
#include "media/base/audio_sample_types.h"

namespace content {

AudioTrackPcmEncoder::AudioTrackPcmEncoder(
    const OnEncodedAudioCB on_encoded_audio_cb)
    : AudioTrackEncoder(on_encoded_audio_cb) {}

void AudioTrackPcmEncoder::OnSetFormat(
    const media::AudioParameters& input_params) {
  DVLOG(1) << __func__;
  DCHECK(encoder_thread_checker_.CalledOnValidThread());
  if (input_params_.Equals(input_params))
    return;

  if (!input_params.IsValid()) {
    DLOG(ERROR) << "Invalid params: " << input_params.AsHumanReadableString();
    return;
  }
  input_params_ = input_params;
  DVLOG(1) << "|input_params_|:" << input_params_.AsHumanReadableString();
}

void AudioTrackPcmEncoder::EncodeAudio(
    std::unique_ptr<media::AudioBus> input_bus,
    const base::TimeTicks& capture_time) {
  DVLOG(3) << __func__ << ", #frames " << input_bus->frames();
  DCHECK(encoder_thread_checker_.CalledOnValidThread());
  DCHECK_EQ(input_bus->channels(), input_params_.channels());
  DCHECK(!capture_time.is_null());

  if (paused_)
    return;

  std::vector<float> buffer(input_bus->frames() * input_bus->channels());
  input_bus->ToInterleaved<media::Float32SampleTypeTraits>(input_bus->frames(),
                                                           &buffer[0]);

  std::unique_ptr<std::string> encoded_data_string(new std::string());
  const size_t bytes_per_sample = sizeof(buffer[0]);
  encoded_data_string->resize(buffer.size() * bytes_per_sample);
  char* encoded_data_ptr = base::string_as_array(encoded_data_string.get());
  for (size_t i = 0; i < buffer.size(); ++i) {
    uint32_t int_sample;
    memcpy(&int_sample, &buffer[i], bytes_per_sample);
    uint32_t swapped_int_sample = base::ByteSwapToLE32(int_sample);
    memcpy(encoded_data_ptr + i * bytes_per_sample, &swapped_int_sample,
           bytes_per_sample);
  }

  const base::TimeTicks capture_time_of_first_sample =
      capture_time -
      base::TimeDelta::FromMicroseconds(input_bus->frames() *
                                        base::Time::kMicrosecondsPerSecond /
                                        input_params_.sample_rate());
  on_encoded_audio_cb_.Run(input_params_, std::move(encoded_data_string),
                           capture_time_of_first_sample);
}

}  // namespace content
