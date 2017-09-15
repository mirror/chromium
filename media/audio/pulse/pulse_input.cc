// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/audio/pulse/pulse_input.h"

#include <stdint.h>

#include "base/logging.h"
#include "media/audio/audio_device_description.h"
#include "media/audio/pulse/audio_manager_pulse.h"
#include "media/audio/pulse/pulse_util.h"
#include "media/base/audio_timestamp_helper.h"

namespace media {

using pulse::AutoPulseLock;

// Number of blocks of buffers used in the |fifo_|.
const int kNumberOfBlocksBufferInFifo = 2;

PulseAudioInputStream::PulseAudioInputStream(AudioManagerPulse* audio_manager,
                                             pulse::Glue* glue,
                                             const std::string& device_name,
                                             const AudioParameters& params)
    : audio_manager_(audio_manager),
      callback_(nullptr),
      device_name_(device_name),
      params_(params),
      channels_(0),
      volume_(0.0),
      stream_started_(false),
      muted_(false),
      fifo_(params.channels(),
            params.frames_per_buffer(),
            kNumberOfBlocksBufferInFifo),
      glue_(glue) {
  DCHECK(glue->pa_mainloop());
  DCHECK(glue->pa_context());
  DCHECK(params_.IsValid());
}

PulseAudioInputStream::~PulseAudioInputStream() {
  // All internal structures should already have been freed in Close(),
  // which calls AudioManagerPulse::Release which deletes this object.
  DCHECK(!stream_);
}

bool PulseAudioInputStream::Open() {
  DCHECK(thread_checker_.CalledOnValidThread());

  // Set sample specifications.
  pa_sample_spec sample_specifications;
  sample_specifications.format = BitsToPASampleFormat(params.bits_per_sample());
  sample_specifications.rate = params.sample_rate();
  sample_specifications.channels = params.channels();

  // Set server-side capture buffer metrics. Detailed documentation on what
  // values should be chosen can be found at
  // freedesktop.org/software/pulseaudio/doxygen/structpa__buffer__attr.html.
  pa_buffer_attr buffer_attributes;
  const int buffer_size = params.GetBytesPerBuffer();
  buffer_attributes.maxlength = static_cast<uint32_t>(-1);
  buffer_attributes.tlength = buffer_size;
  buffer_attributes.minreq = buffer_size;
  buffer_attributes.prebuf = static_cast<uint32_t>(-1);
  buffer_attributes.fragsize = buffer_size;

  const pa_stream_flags_t flags =
      PA_STREAM_AUTO_TIMING_UPDATE | PA_STREAM_INTERPOLATE_TIMING |
      PA_STREAM_ADJUST_LATENCY | PA_STREAM_START_CORKED;

  auto stream = std::make_unique<pulse::Stream>(
      StreamType::INPUT, glue_, params_, device_id_, &StreamNotifyCallback,
      &ReadCallback, flags, &sample_specifications, &buffer_attributes, this);
  if (stream->has_error())
    return false;

  stream_ = std::move(stream);
  return true;
}

void PulseAudioInputStream::Start(AudioInputCallback* callback) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(callback);
  DCHECK(pa_stream_);
  if (stream_started_)
    return;

  // Start the streaming.
  StartAgc();
  callback_ = callback;
  stream_started_ = true;
  if (stream_->Start())
    return;

  DCHECK(stream_->has_error());
  stream_->Stop();
  callback_ = nullptr;
  callback->OnError();
}

void PulseAudioInputStream::Stop() {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (!stream_started_)
    return;

  StopAgc();

  if (!stream_->Stop())
    callback_->OnError();

  pa_stream_drop(stream_->pa_stream());
  stream_started_ = false;
  fifo_.Clear();
  callback_ = nullptr;
}

void PulseAudioInputStream::Close() {
  DCHECK(thread_checker_.CalledOnValidThread());
  stream_.reset();

  // Signal to the manager that we're closed and can be removed.
  // This should be the last call in the function as it deletes "this".
  audio_manager_->ReleaseInputStream(this);
}

double PulseAudioInputStream::GetMaxVolume() {
  return static_cast<double>(PA_VOLUME_NORM);
}

void PulseAudioInputStream::SetVolume(double volume) {
  AutoPulseLock auto_lock(pa_mainloop_);
  if (!stream_ || !stream_->pa_stream())
    return;

  size_t index = pa_stream_get_device_index(stream_->pa_stream());
  if (!channels_) {
    // Get the number of channels for the source only when the |channels_| is 0.
    // We are assuming the stream source is not changed on the fly here.
    glue_->WaitForOperationCompletion(pa_context_get_source_info_by_index(
        glue_->pa_context(), index, &VolumeCallback, this));
    if (!channels_) {
      DLOG(WARNING) << "Failed to get the number of channels for the source";
      return;
    }
  }

  pa_cvolume pa_volume;
  pa_cvolume_set(&pa_volume, channels_, volume);

  // Don't need to wait for this task to complete.
  pa_operation_unref(pa_context_set_source_volume_by_index(
      glue_->pa_context(), index, &pa_volume, nullptr, nullptr));
}

double PulseAudioInputStream::GetVolume() {
  if (pa_threaded_mainloop_in_thread(glue_->pa_mainloop())) {
    // When being called by the pulse thread, GetVolume() is asynchronous and
    // called under AutoPulseLock.
    if (!stream_ || !stream_->pa_stream())
      return 0.0;

    // Do not wait for the operation since we can't block the pulse thread.
    pa_operation_unref(pa_context_get_source_info_by_index(
        glue_->pa_context(), pa_stream_get_device_index(stream_->pa_stream()),
        &VolumeCallback, this));

    // Return zero and the callback will asynchronously update the |volume_|.
    return 0.0;
  }

  GetSourceInformation(&VolumeCallback);
  return volume_;
}

bool PulseAudioInputStream::IsMuted() {
  DCHECK(thread_checker_.CalledOnValidThread());
  GetSourceInformation(&MuteCallback);
  return muted_;
}

// static, used by pa_stream_set_read_callback.
void PulseAudioInputStream::ReadCallback(pa_stream* handle,
                                         size_t length,
                                         void* user_data) {
  PulseAudioInputStream* stream =
      reinterpret_cast<PulseAudioInputStream*>(user_data);

  stream->ReadData();
}

// static, used by pa_context_get_source_info_by_index.
void PulseAudioInputStream::VolumeCallback(pa_context* context,
                                           const pa_source_info* info,
                                           int error,
                                           void* user_data) {
  PulseAudioInputStream* stream =
      reinterpret_cast<PulseAudioInputStream*>(user_data);

  if (error) {
    pa_threaded_mainloop_signal(stream->glue_->pa_mainloop(), 0);
    return;
  }

  if (stream->channels_ != info->channel_map.channels)
    stream->channels_ = info->channel_map.channels;

  pa_volume_t volume = PA_VOLUME_MUTED;  // Minimum possible value.
  // Use the max volume of any channel as the volume.
  for (int i = 0; i < stream->channels_; ++i) {
    if (volume < info->volume.values[i])
      volume = info->volume.values[i];
  }

  // It is safe to access |volume_| here since VolumeCallback() is running
  // under PulseLock.
  stream->volume_ = static_cast<double>(volume);
}

// static, used by pa_context_get_source_info_by_index.
void PulseAudioInputStream::MuteCallback(pa_context* context,
                                         const pa_source_info* info,
                                         int error,
                                         void* user_data) {
  // Runs on PulseAudio callback thread. It might be possible to make this
  // method more thread safe by passing a struct (or pair) of a local copy of
  // |pa_mainloop_| and |muted_| instead.
  PulseAudioInputStream* stream =
      reinterpret_cast<PulseAudioInputStream*>(user_data);

  // Avoid infinite wait loop in case of error.
  if (error) {
    pa_threaded_mainloop_signal(stream->glue_->pa_mainloop(), 0);
    return;
  }

  stream->muted_ = info->mute != 0;
}

// static, used by pa_stream_set_state_callback.
void PulseAudioInputStream::StreamNotifyCallback(pa_stream* s,
                                                 void* user_data) {
  PulseAudioInputStream* stream =
      reinterpret_cast<PulseAudioInputStream*>(user_data);

  if (s && stream->callback_ && pa_stream_get_state(s) == PA_STREAM_FAILED) {
    stream->callback_->OnError(stream);
  }

  pa_threaded_mainloop_signal(stream->glue_->pa_mainloop(), 0);
}

void PulseAudioInputStream::ReadData() {
  // Update the AGC volume level once every second. Note that,
  // |volume| is also updated each time SetVolume() is called
  // through IPC by the render-side AGC.
  // We disregard the |normalized_volume| from GetAgcVolume()
  // and use the value calculated by |volume_|.
  double normalized_volume = 0.0;
  GetAgcVolume(&normalized_volume);
  normalized_volume = volume_ / GetMaxVolume();

  // Compensate the audio delay caused by the FIFO.
  // TODO(dalecurtis): This should probably use pa_stream_get_time() so we can
  // get the capture time directly.
  base::TimeTicks capture_time =
      base::TimeTicks::Now() -
      (pulse::GetHardwareLatency(stream_->pa_stream()) +
       AudioTimestampHelper::FramesToTime(fifo_.GetAvailableFrames(),
                                          params_.sample_rate()));
  do {
    size_t length = 0;
    const void* data = NULL;
    pa_stream_peek(stream_->pa_stream(), &data, &length);
    if (!data || length == 0)
      break;

    const int number_of_frames = length / params_.GetBytesPerFrame();
    if (number_of_frames > fifo_.GetUnfilledFrames()) {
      // Dynamically increase capacity to the FIFO to handle larger buffer got
      // from Pulse.
      const int increase_blocks_of_buffer =
          static_cast<int>((number_of_frames - fifo_.GetUnfilledFrames()) /
                           params_.frames_per_buffer()) +
          1;
      fifo_.IncreaseCapacity(increase_blocks_of_buffer);
    }

    fifo_.Push(data, number_of_frames, params_.bits_per_sample() / 8);

    // Checks if we still have data.
    pa_stream_drop(stream_->pa_stream());
  } while (pa_stream_readable_size(stream_->pa_stream()) > 0);

  while (fifo_.available_blocks()) {
    const AudioBus* audio_bus = fifo_.Consume();

    callback_->OnData(this, audio_bus, capture_time, normalized_volume);

    // Move the capture time forward for each vended block.
    capture_time += AudioTimestampHelper::FramesToTime(audio_bus->frames(),
                                                       params_.sample_rate());

    // Sleep 5ms to wait until render consumes the data in order to avoid
    // back to back OnData() method.
    // TODO(dalecurtis): Delete all this. It shouldn't be necessary now that we
    // have a ring buffer and FIFO on the actual shared memory.,
    if (fifo_.available_blocks())
      base::PlatformThread::Sleep(base::TimeDelta::FromMilliseconds(5));
  }

  pa_threaded_mainloop_signal(glue_->pa_mainloop(), 0);
}

bool PulseAudioInputStream::GetSourceInformation(pa_source_info_cb_t callback) {
  if (!stream_ || !stream_->pa_stream())
    return false;

  AutoPulseLock auto_lock(glue_->pa_mainloop());
  glue_->WaitForOperationCompletion(pa_context_get_source_info_by_index(
      glue_->pa_context(), pa_stream_get_device_index(stream_->pa_stream()),
      callback, this));
  return true;
}

}  // namespace media
