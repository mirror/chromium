// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_MEDIA_CMA_BACKEND_AUDIO_OUTPUT_STREAM_FUCHSIA_H_
#define CHROMECAST_MEDIA_CMA_BACKEND_AUDIO_OUTPUT_STREAM_FUCHSIA_H_

#include "chromecast/media/cma/backend/audio_output_stream.h"

#include <media/audio.h>

namespace chromecast {
namespace media {

// AudioOutputStream implementation for Fuchsia.
class AudioOutputStreamFuchsia : public AudioOutputStream {
 public:
  AudioOutputStreamFuchsia();
  ~AudioOutputStreamFuchsia() override;

  // AudioOutputStream interface.
  bool IsFixedSampleRate() override;
  bool Start(int requested_sample_rate, int channels) override;
  bool GetTimeUntilUnderrun(base::TimeDelta* result) override;
  int GetSampleRate() override;
  MediaPipelineBackend::AudioDecoder::RenderingDelay GetRenderingDelay()
      override;
  bool Write(const float* data,
             int data_size,
             bool* out_playback_interrupted) override;
  void Stop() override;

 private:
  bool UpdatePresentatioDelay();
  base::TimeTicks GetCurrentStreamTime();

  fuchsia_audio_manager* manager_ = nullptr;
  fuchsia_audio_output_stream* stream_ = nullptr;

  int sample_rate_ = 0;
  int channels_ = 0;

  base::TimeTicks started_time_;
  int64_t stream_position_samples_ = 0;

  // Total presentation delay for the stream. This value is returned by
  // fuchsia_audio_output_stream_get_min_delay()
  zx_duration_t presentation_delay_ns_ = 0;

  DISALLOW_COPY_AND_ASSIGN(AudioOutputStreamFuchsia);
};

}  // namespace media
}  // namespace chromecast

#endif  // CHROMECAST_MEDIA_CMA_BACKEND_AUDIO_OUTPUT_STREAM_FUCHSIA_H_
