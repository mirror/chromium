// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_AUDIO_FUCHSIA_AUDIO_OUTPUT_STREAM_FUCHSIA_H_
#define MEDIA_AUDIO_FUCHSIA_AUDIO_OUTPUT_STREAM_FUCHSIA_H_

#include <media/audio.h>

#include "base/timer/timer.h"
#include "media/audio/audio_io.h"
#include "media/base/audio_parameters.h"

namespace media {

class AudioOutputStreamFuchsia : public AudioOutputStream {
 public:
  // Caller must ensure that manager outlives the stream.
  AudioOutputStreamFuchsia(fuchsia_audio_manager* manager,
                           const std::string& device_id,
                           const AudioParameters& parameters);
  ~AudioOutputStreamFuchsia() override;

  // AudioOutputStream interface.
  bool Open() override;
  void Start(AudioSourceCallback* callback) override;
  void Stop() override;
  void SetVolume(double volume) override;
  void GetVolume(double* volume) override;
  void Close() override;

 private:
  // Requests data from AudioSourceCallback, passes it to the mixer and
  // schedules |timer_| for the next call.
  void PumpSamples();

  base::TimeTicks GetCurrentStreamTime();

  fuchsia_audio_manager* manager_;
  std::string device_id_;
  AudioParameters parameters_;

  fuchsia_audio_output_stream* stream_ = nullptr;
  AudioSourceCallback* callback_ = nullptr;

  double volume_ = 1.0;

  // Time the stream was started. Set to null if the stream is not running.
  base::TimeTicks started_time_;

  // Total presentation delay for the stream. This value is returned by
  // fuchsia_audio_output_stream_get_min_delay()
  base::TimeDelta presentation_delay_;

  int64_t stream_position_samples_ = 0;

  // Timer that's scheduled to call PumpSamples().
  base::OneShotTimer timer_;

  DISALLOW_COPY_AND_ASSIGN(AudioOutputStreamFuchsia);
};

}  // namespace media

#endif  // MEDIA_AUDIO_FUCHSIA_AUDIO_OUTPUT_STREAM_FUCHSIA_H_
