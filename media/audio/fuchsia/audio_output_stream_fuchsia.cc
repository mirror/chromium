// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/audio/fuchsia/audio_output_stream_fuchsia.h"

#include <media/audio.h>

#include "media/audio/fuchsia/audio_manager_fuchsia.h"
#include "media/base/audio_sample_types.h"
#include "media/base/audio_timestamp_helper.h"

namespace media {

AudioOutputStreamFuchsia::AudioOutputStreamFuchsia(
    fuchsia_audio_manager* manager,
    const std::string& device_id,
    const AudioParameters& parameters)
    : manager_(manager),
      device_id_(device_id),
      parameters_(parameters),
      audio_bus_(AudioBus::Create(parameters)),
      buffer_(parameters_.frames_per_buffer() * parameters_.channels()) {}

AudioOutputStreamFuchsia::~AudioOutputStreamFuchsia() {
  Close();
}

bool AudioOutputStreamFuchsia::Open() {
  DCHECK(!stream_);

  fuchsia_audio_parameters fuchsia_params;
  fuchsia_params.sample_rate = parameters_.sample_rate();
  fuchsia_params.num_channels = parameters_.channels();
  fuchsia_params.buffer_size = parameters_.frames_per_buffer();

  int result = fuchsia_audio_manager_create_output_stream(
      manager_, const_cast<char*>(device_id_.c_str()), &fuchsia_params,
      &stream_);
  if (result < 0) {
    DLOG(ERROR) << "Failed to open audio output " << device_id_
                << " error code: " << result;
    DCHECK(!stream_);
    return false;
  }

  return true;
}

void AudioOutputStreamFuchsia::Start(AudioSourceCallback* callback) {
  DCHECK(!callback_);
  DCHECK(started_time_.is_null());
  callback_ = callback;

  PumpSamples();
}

void AudioOutputStreamFuchsia::Stop() {
  DCHECK(callback_);
  callback_ = nullptr;
  started_time_ = base::TimeTicks();
  timer_.Stop();
}

void AudioOutputStreamFuchsia::SetVolume(double volume) {
  DCHECK(0.0 <= volume && volume <= 1.0) << volume;
  volume_ = volume;
}

void AudioOutputStreamFuchsia::GetVolume(double* volume) {
  *volume = volume_;
}

void AudioOutputStreamFuchsia::Close() {
  Stop();

  if (stream_) {
    fuchsia_audio_output_stream_free(stream_);
    stream_ = nullptr;
  }
}


base::TimeTicks AudioOutputStreamFuchsia::GetCurrentStreamTime() {
  DCHECK(!started_time_.is_null());
  return started_time_ +
         AudioTimestampHelper::FramesToTime(stream_position_samples_,
                                           parameters_.sample_rate());
}

void AudioOutputStreamFuchsia::PumpSamples() {
  DCHECK(stream_);

  // Presentation time (PTS) has to be specified only for the first frame after
  // stream is started or restarted. Mixer will calculate PTS for all following
  // frames.
  zx_time_t presentation_time = 0;

  // If the stream wasn't running before then initialize position.
  if (started_time_.is_null()) {
    // TODO(sergeyu): Call fuchsia_audio_output_stream_get_min_delay()
    // periodically to handle the case when minimum delay changes (e.g. if the
    // stream is redirected to a different device).
    zx_duration_t min_delay_ns;
    int result =
        fuchsia_audio_output_stream_get_min_delay(stream_, &min_delay_ns);
    if (result != ZX_OK) {
      DLOG(ERROR) << "fuchsia_audio_output_stream_get_min_delay() failed: "
                  << result;
      callback_->OnError();
      return;
    }

    started_time_ = base::TimeTicks::Now();
    stream_position_samples_ = 0;
    presentation_time = zx_time_get(ZX_CLOCK_MONOTONIC) + min_delay_ns;
    presentation_delay_ =
        base::TimeDelta::FromMicroseconds(min_delay_ns / 1000);
  }

  int frames_filled = callback_->OnMoreData(
      presentation_delay_, GetCurrentStreamTime(), 0, audio_bus_.get());

  DCHECK_GE(frames_filled, 0);
  if (frames_filled == 0) {
    // Stream is exhausted. Reset the position and schedule a timer to call
    // OnMoreData() again in 10ms.
    started_time_ = base::TimeTicks();
    timer_.Start(FROM_HERE, base::TimeDelta::FromMilliseconds(10),
                 base::Bind(&AudioOutputStreamFuchsia::PumpSamples,
                            base::Unretained(this)));
    return;
  }

  audio_bus_->Scale(volume_);
  audio_bus_->ToInterleaved<media::Float32SampleTypeTraits>(frames_filled,
                                                            buffer_.data());

  int result = fuchsia_audio_output_stream_write(
      stream_, buffer_.data(), frames_filled, presentation_time);
  if (result != ZX_OK) {
    DLOG(ERROR) << "fuchsia_audio_output_stream_write() returned " << result;
    callback_->OnError();
    return;
  }

  stream_position_samples_ += frames_filled;

  DCHECK_GT(result, 0);

  timer_.Start(FROM_HERE, GetCurrentStreamTime() - base::TimeTicks::Now(),
               base::Bind(&AudioOutputStreamFuchsia::PumpSamples,
                          base::Unretained(this)));
}

}  // namespace media
