// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/audio/pulse/pulse_output.h"

#include <pulse/pulseaudio.h>
#include <stdint.h>

#include "base/single_thread_task_runner.h"
#include "base/time/time.h"
#include "media/audio/audio_device_description.h"
#include "media/audio/audio_manager_base.h"
#include "media/audio/pulse/pulse_util.h"
#include "media/base/audio_sample_types.h"

namespace media {

using pulse::AutoPulseLock;

// static, pa_stream_notify_cb
void PulseAudioOutputStream::StreamNotifyCallback(pa_stream* s, void* p_this) {
  PulseAudioOutputStream* stream = static_cast<PulseAudioOutputStream*>(p_this);

  // Forward unexpected failures to the AudioSourceCallback if available.  All
  // these variables are only modified under pa_threaded_mainloop_lock() so this
  // should be thread safe.
  if (s && stream->source_callback_ &&
      pa_stream_get_state(s) == PA_STREAM_FAILED) {
    stream->source_callback_->OnError();
  }

  pa_threaded_mainloop_signal(stream->glue_->pa_mainloop(), 0);
}

// static, pa_stream_request_cb_t
void PulseAudioOutputStream::StreamRequestCallback(pa_stream* s,
                                                   size_t len,
                                                   void* p_this) {
  // Fulfill write request; must always result in a pa_stream_write() call.
  static_cast<PulseAudioOutputStream*>(p_this)->FulfillWriteRequest(len);
}

PulseAudioOutputStream::PulseAudioOutputStream(const AudioParameters& params,
                                               const std::string& device_id,
                                               AudioManagerBase* manager)
    : params_(AudioParameters(params.format(),
                              params.channel_layout(),
                              params.sample_rate(),
                              // Ignore the given bits per sample. We
                              // want 32 because we're outputting
                              // floats.
                              32,
                              params.frames_per_buffer())),
      device_id_(device_id),
      manager_(manager),
      volume_(1.0f),
      source_callback_(nullptr) {
  CHECK(params_.IsValid());
  audio_bus_ = AudioBus::Create(params_);
}

PulseAudioOutputStream::~PulseAudioOutputStream() {
  // All internal structures should already have been freed in Close(), which
  // calls AudioManagerBase::ReleaseOutputStream() which deletes this object.
  DCHECK(!stream_);
  DCHECK(!glue_);
}

bool PulseAudioOutputStream::Open() {
  DCHECK(thread_checker_.CalledOnValidThread());

  auto glue = std::make_unique<pulse::Glue>();
  if (!glue->pa_mainloop())
    return false;

  // Set sample specifications.
  pa_sample_spec sample_specifications;
  sample_specifications.format = PA_SAMPLE_FLOAT32;
  sample_specifications.rate = params.sample_rate();
  sample_specifications.channels = params.channels();

  // Pulse is very finicky with the small buffer sizes used by Chrome.  The
  // settings below are mostly found through trial and error.  Essentially we
  // want Pulse to auto size its internal buffers, but call us back nearly every
  // |minreq| bytes.  |tlength| should be a multiple of |minreq|; too low and
  // Pulse will issue callbacks way too fast, too high and we don't get
  // callbacks frequently enough.
  //
  // Setting |minreq| to the exact buffer size leads to more callbacks than
  // necessary, so we've clipped it to half the buffer size.  Regardless of the
  // requested amount, we'll always fill |params.GetBytesPerBuffer()| though.
  pa_buffer_attr pa_buffer_attributes;
  pa_buffer_attributes.maxlength = static_cast<uint32_t>(-1);
  pa_buffer_attributes.minreq = params.GetBytesPerBuffer() / 2;
  pa_buffer_attributes.prebuf = static_cast<uint32_t>(-1);
  pa_buffer_attributes.tlength = params.GetBytesPerBuffer() * 3;
  pa_buffer_attributes.fragsize = static_cast<uint32_t>(-1);

  const pa_stream_flags_t flags =
      PA_STREAM_INTERPOLATE_TIMING | PA_STREAM_ADJUST_LATENCY |
      PA_STREAM_AUTO_TIMING_UPDATE | PA_STREAM_NOT_MONOTONIC |
      PA_STREAM_START_CORKED;

  auto stream = std::make_unique<pulse::Stream>(
      StreamType::OUTPUT, glue.get(), params_, device_id_,
      &StreamNotifyCallback, &StreamRequestCallback, flags,
      &sample_specifications, &buffer_attributes, this);
  if (stream->has_error())
    return false;

  glue_ = std::move(glue);
  stream_ = std::move(stream);
  return true;
}

void PulseAudioOutputStream::Close() {
  DCHECK(thread_checker_.CalledOnValidThread());

  stream_.reset();
  glue_.reset();

  // Signal to the manager that we're closed and can be removed.
  // This should be the last call in the function as it deletes "this".
  manager_->ReleaseOutputStream(this);
}

void PulseAudioOutputStream::FulfillWriteRequest(size_t requested_bytes) {
  int bytes_remaining = requested_bytes;
  while (bytes_remaining > 0) {
    void* buffer = NULL;
    size_t bytes_to_fill = params_.GetBytesPerBuffer();
    CHECK_GE(
        pa_stream_begin_write(stream_->pa_stream(), &buffer, &bytes_to_fill),
        0);
    CHECK_EQ(bytes_to_fill, static_cast<size_t>(params_.GetBytesPerBuffer()));

    // NOTE: |bytes_to_fill| may be larger than |requested_bytes| now, this is
    // okay since pa_stream_begin_write() is the authoritative source on how
    // much can be written.

    int frames_filled = 0;
    if (source_callback_) {
      const base::TimeDelta delay =
          pulse::GetHardwareLatency(stream_->pa_stream());
      frames_filled = source_callback_->OnMoreData(
          delay, base::TimeTicks::Now(), 0, audio_bus_.get());

      // Zero any unfilled data so it plays back as silence.
      if (frames_filled < audio_bus_->frames()) {
        audio_bus_->ZeroFramesPartial(frames_filled,
                                      audio_bus_->frames() - frames_filled);
      }

      audio_bus_->Scale(volume_);
      audio_bus_->ToInterleaved<Float32SampleTypeTraits>(
          audio_bus_->frames(), reinterpret_cast<float*>(buffer));
    } else {
      memset(buffer, 0, bytes_to_fill);
    }

    if (pa_stream_write(stream_->pa_stream(), buffer, bytes_to_fill, NULL, 0LL,
                        PA_SEEK_RELATIVE) < 0) {
      if (source_callback_) {
        source_callback_->OnError();
      }
    }

    // NOTE: As mentioned above, |bytes_remaining| may be negative after this.
    bytes_remaining -= bytes_to_fill;

    // Despite telling Pulse to only request certain buffer sizes, it will not
    // always obey.  In these cases we need to avoid back to back reads from
    // the renderer as it won't have time to complete the request.
    //
    // We can't defer the callback as Pulse will never call us again until we've
    // satisfied writing the requested number of bytes.
    //
    // TODO(dalecurtis): It might be worth choosing the sleep duration based on
    // the hardware latency return above.  Watch http://crbug.com/366433 to see
    // if a more complicated wait process is necessary.  We may also need to see
    // if a PostDelayedTask should be used here to avoid blocking the PulseAudio
    // command thread.
    if (source_callback_ && bytes_remaining > 0)
      base::PlatformThread::Sleep(params_.GetBufferDuration() / 4);
  }
}

void PulseAudioOutputStream::Start(AudioSourceCallback* callback) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(callback);
  DCHECK(stream_);

  source_callback_ = callback;
  if (stream_->Start())
    return;

  DCHECK(stream_->has_error());
  stream_->Stop();
  source_callback_ = nullptr;
  callback->OnError();
}

void PulseAudioOutputStream::Stop() {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (!stream_->Stop())
    source_callback_->OnError();
  source_callback_ = nullptr;
}

void PulseAudioOutputStream::SetVolume(double volume) {
  DCHECK(thread_checker_.CalledOnValidThread());

  // Waiting for the main loop lock will ensure outstanding callbacks have
  // completed and |volume_| is not accessed from them.
  AutoPulseLock auto_lock(pa_mainloop_);
  volume_ = static_cast<float>(volume);
}

void PulseAudioOutputStream::GetVolume(double* volume) {
  DCHECK(thread_checker_.CalledOnValidThread());

  *volume = volume_;
}

}  // namespace media
