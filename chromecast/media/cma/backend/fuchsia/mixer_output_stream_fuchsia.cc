// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/media/cma/backend/fuchsia/mixer_output_stream_fuchsia.h"

#include <media/audio.h>
#include <zircon/syscalls.h>

#include "base/command_line.h"
#include "base/time/time.h"
#include "chromecast/base/chromecast_switches.h"
#include "media/base/audio_sample_types.h"
#include "media/base/audio_timestamp_helper.h"
#include "media/base/media_switches.h"

namespace chromecast {
namespace media {

// Always use 48kHz sample rate for the output stream. Currently 48kHz sample
// rate has to be used for 2 reasons:
//   1. Some loopback consumers assume that loopback is always sampled at 48kHz.
//      See b/38282085.
//   2. Fuchsia's mixer normally resamples to 48kHz and it uses low quality
//      bilinear resampler, see MTWN-45.
// StreamMixer will resample all output streams to this sample rate.
constexpr int kSampleRate = 48000;

// |buffer_size| passed to media_client library when initializing audio output
// stream. Current implementation ignores this parameter, so the value doesn't
// make much difference. StreamMixer by default writes chunks of 768 frames.
constexpr int kDefaultPeriodSize = 768;

// Maximum buffer duration. This value is used to simulate blocking Write()
// similar to ALSA's behavior, see comments in
// MixerOutputStreamFuchsia::Write().
constexpr base::TimeDelta kMaxOutputBufferDuration =
    base::TimeDelta::FromMilliseconds(40);

// static
std::unique_ptr<MixerOutputStream> MixerOutputStream::Create() {
  return std::make_unique<MixerOutputStreamFuchsia>();
}

MixerOutputStreamFuchsia::MixerOutputStreamFuchsia() {}

MixerOutputStreamFuchsia::~MixerOutputStreamFuchsia() {
  if (manager_)
    fuchsia_audio_manager_free(manager_);
}

bool MixerOutputStreamFuchsia::IsFixedSampleRate() {
  return true;
}

bool MixerOutputStreamFuchsia::Start(int requested_sample_rate, int channels) {
  DCHECK(!stream_);

  if (!manager_)
    manager_ = fuchsia_audio_manager_create();

  DCHECK(manager_);

  fuchsia_audio_parameters fuchsia_params;
  fuchsia_params.sample_rate = kSampleRate;
  fuchsia_params.num_channels = channels;
  fuchsia_params.buffer_size = kDefaultPeriodSize;

  int result = fuchsia_audio_manager_create_output_stream(
      manager_, nullptr, &fuchsia_params, &stream_);
  if (result < 0) {
    LOG(ERROR) << "Failed to open audio output, error code: " << result;
    DCHECK(!stream_);
    return false;
  }

  if (!UpdatePresentationDelay()) {
    fuchsia_audio_output_stream_free(stream_);
    stream_ = nullptr;
    return false;
  }

  sample_rate_ = kSampleRate;
  channels_ = channels;

  started_time_ = base::TimeTicks();

  return true;
}

int MixerOutputStreamFuchsia::GetSampleRate() {
  return sample_rate_;
}

MediaPipelineBackend::AudioDecoder::RenderingDelay
MixerOutputStreamFuchsia::GetRenderingDelay() {
  base::TimeTicks now = base::TimeTicks::Now();
  base::TimeDelta delay =
      base::TimeDelta::FromMicroseconds(presentation_delay_ns_ / 1000);
  if (!started_time_.is_null()) {
    base::TimeTicks stream_time = GetCurrentStreamTime();
    if (stream_time > now)
      delay += stream_time - now;
  }

  return MediaPipelineBackend::AudioDecoder::RenderingDelay(
      /*delay_microseconds=*/delay.InMicroseconds(),
      /*timestamp_microseconds=*/(now - base::TimeTicks()).InMicroseconds());
}

bool MixerOutputStreamFuchsia::GetTimeUntilUnderrun(base::TimeDelta* result) {
  if (!started_time_.is_null()) {
    *result = GetCurrentStreamTime() - base::TimeTicks::Now();
  } else {
    *result = base::TimeDelta();
  }
  return true;
}

bool MixerOutputStreamFuchsia::Write(const float* data,
                                     int data_size,
                                     bool* out_playback_interrupted) {
  if (!stream_)
    return false;

  DCHECK(data_size % channels_ == 0);

  do {
    zx_time_t presentation_time = FUCHSIA_AUDIO_NO_TIMESTAMP;
    if (started_time_.is_null()) {
      // Presentation time (PTS) needs to be specified only for the first frame
      // after stream is started or restarted. Mixer will calculate PTS for all
      // following frames. 1us is added to account for the time passed between
      // zx_time_get() and fuchsia_audio_output_stream_write().
      zx_time_t zx_now = zx_time_get(ZX_CLOCK_MONOTONIC);
      presentation_time = zx_now + presentation_delay_ns_ + ZX_USEC(1);
      started_time_ = base::TimeTicks::FromZxTime(zx_now);
      stream_position_samples_ = 0;
    }
    int result = fuchsia_audio_output_stream_write(
        stream_, const_cast<float*>(data), data_size, presentation_time);
    if (result == ZX_ERR_IO_MISSED_DEADLINE) {
      LOG(ERROR) << "MixerOutputStreamFuchsia::PumpSamples() missed deadline, "
                    "resetting PTS.";
      if (!UpdatePresentationDelay()) {
        return false;
      }
      started_time_ = base::TimeTicks();
      *out_playback_interrupted = true;
    } else if (result != ZX_OK) {
      LOG(ERROR) << "fuchsia_audio_output_stream_write() returned " << result;
      return false;
    }

  } while (started_time_.is_null());

  int frames = data_size / channels_;
  stream_position_samples_ += frames;

  // Block the thread to limit amount of buffered data. Currently
  // MixerOutputStreamAlsa uses blocking Write() and StreamMixer relies on that
  // behavior. Sleep() below replicates the same behavior on Fuchsia.
  // TODO(sergeyu): Refactor StreamMixer to work with non-blocking Write().
  base::TimeDelta current_buffer_duration =
      GetCurrentStreamTime() - base::TimeTicks::Now();
  if (current_buffer_duration > kMaxOutputBufferDuration) {
    base::PlatformThread::Sleep(current_buffer_duration -
                                kMaxOutputBufferDuration);
  }

  return true;
}

void MixerOutputStreamFuchsia::Stop() {
  started_time_ = base::TimeTicks();

  if (stream_) {
    fuchsia_audio_output_stream_free(stream_);
    stream_ = nullptr;
  }
}

bool MixerOutputStreamFuchsia::UpdatePresentationDelay() {
  int result = fuchsia_audio_output_stream_get_min_delay(
      stream_, &presentation_delay_ns_);
  if (result != ZX_OK) {
    LOG(ERROR) << "fuchsia_audio_output_stream_get_min_delay() failed: "
               << result;
    return false;
  }

  return true;
}

base::TimeTicks MixerOutputStreamFuchsia::GetCurrentStreamTime() {
  DCHECK(!started_time_.is_null());
  return started_time_ + ::media::AudioTimestampHelper::FramesToTime(
                             stream_position_samples_, sample_rate_);
}

}  // namespace media
}  // namespace chromecast
