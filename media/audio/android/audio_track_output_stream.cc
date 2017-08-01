// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/audio/android/audio_track_output_stream.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/logging.h"
#include "base/single_thread_task_runner.h"
#include "base/time/default_tick_clock.h"
#include "jni/AudioTrackOutputStream_jni.h"
#include "media/audio/audio_manager_base.h"
#include "media/base/audio_timestamp_helper.h"

using base::android::AttachCurrentThread;

namespace media {

// Android audio format. For more information, please see:
// https://developer.android.com/reference/android/media/AudioFormat.html
enum {
  kEncodingPcm16bit = 2,  // ENCODING_PCM_16BIT
  kEncodingAc3 = 5,       // ENCODING_AC3
  kEncodingEac3 = 6,      // ENCODING_E_AC3
};

AudioTrackOutputStream::AudioTrackOutputStream(AudioManagerBase* manager,
                                               const AudioParameters& params)
    : params_(params),
      audio_manager_(manager),
      audio_bus_(AudioBus::Create(params_)),
      tick_clock_(new base::DefaultTickClock()) {
  audio_bus_->set_is_bitstream_format(params_.IsBitstreamFormat());
}

AudioTrackOutputStream::~AudioTrackOutputStream() {
  DCHECK(!callback_);
}

bool AudioTrackOutputStream::Open() {
  DCHECK(audio_manager_->GetTaskRunner()->BelongsToCurrentThread());
  JNIEnv* env = AttachCurrentThread();
  j_audio_output_stream_.Reset(Java_AudioTrackOutputStream_create(env));

  int format = kEncodingPcm16bit;
  if (params_.IsBitstreamFormat()) {
    if (params_.format() == AudioParameters::AUDIO_BITSTREAM_AC3) {
      format = kEncodingAc3;
    } else if (params_.format() == AudioParameters::AUDIO_BITSTREAM_EAC3) {
      format = kEncodingEac3;
    } else {
      NOTREACHED();
    }
  }

  return Java_AudioTrackOutputStream_open(env, j_audio_output_stream_.obj(),
                                          params_.channels(),
                                          params_.sample_rate(), format);
}

void AudioTrackOutputStream::Start(AudioSourceCallback* callback) {
  DCHECK(audio_manager_->GetTaskRunner()->BelongsToCurrentThread());
  callback_ = callback;
  total_read_frames_ = 0;
  Java_AudioTrackOutputStream_start(AttachCurrentThread(),
                                    j_audio_output_stream_.obj(),
                                    reinterpret_cast<intptr_t>(this));
}

void AudioTrackOutputStream::Stop() {
  DCHECK(audio_manager_->GetTaskRunner()->BelongsToCurrentThread());
  Java_AudioTrackOutputStream_stop(AttachCurrentThread(),
                                   j_audio_output_stream_.obj());
  callback_ = nullptr;
}

void AudioTrackOutputStream::Close() {
  DCHECK(!callback_);
  DCHECK(audio_manager_->GetTaskRunner()->BelongsToCurrentThread());

  Java_AudioTrackOutputStream_close(AttachCurrentThread(),
                                    j_audio_output_stream_.obj());
  audio_manager_->ReleaseOutputStream(this);
}

void AudioTrackOutputStream::SetMute(bool muted) {
  if (muted_ == muted)
    return;

  muted_ = muted;
  Java_AudioTrackOutputStream_setVolume(AttachCurrentThread(),
                                        j_audio_output_stream_.obj(),
                                        muted_ ? 0.0 : volume_);
}

void AudioTrackOutputStream::SetVolume(double volume) {
  // Track |volume_| since AudioTrack uses a scaled value.
  volume_ = volume;
  if (muted_)
    return;

  Java_AudioTrackOutputStream_setVolume(AttachCurrentThread(),
                                        j_audio_output_stream_.obj(), volume);
};

void AudioTrackOutputStream::GetVolume(double* volume) {
  *volume = volume_;
};

// AudioOutputStream::SourceCallback implementation methods called from Java.
int AudioTrackOutputStream::OnMoreData(JNIEnv* env,
                                       jobject obj,
                                       jobject audio_data,
                                       jlong total_played_frames) {
  if (!callback_)
    return 0;

  int64_t delay_in_frame = total_read_frames_ - total_played_frames;
  if (delay_in_frame < 0)
    delay_in_frame = 0;
  base::TimeDelta delay =
      AudioTimestampHelper::FramesToTime(delay_in_frame, params_.sample_rate());

  callback_->OnMoreData(delay, tick_clock_->NowTicks(), 0, audio_bus_.get());

  if (params_.IsBitstreamFormat()) {
    if (audio_bus_->GetBitstreamDataSize() <= 0)
      return 0;
    total_read_frames_ += audio_bus_->GetBitstreamFrames();

    void* native_bus = env->GetDirectBufferAddress(audio_data);
    memcpy(native_bus, audio_bus_->channel(0),
           audio_bus_->GetBitstreamDataSize());
    return audio_bus_->GetBitstreamDataSize();
  } else {
    total_read_frames_ += audio_bus_->frames();

    int16_t* native_bus =
        reinterpret_cast<int16_t*>(env->GetDirectBufferAddress(audio_data));
    for (int ch = 0; ch < audio_bus_->channels(); ++ch) {
      const float* source_data = audio_bus_->channel(ch);
      for (int i = 0, offset = ch; i < audio_bus_->frames();
           ++i, offset += audio_bus_->channels()) {
        native_bus[offset] = 32767 * source_data[i];
      }
    }
    return sizeof(*native_bus) * audio_bus_->channels() * audio_bus_->frames();
  }
}

void AudioTrackOutputStream::OnError(JNIEnv* env, jobject obj) {
  DCHECK(callback_);
  callback_->OnError();
}

// static
bool AudioTrackOutputStream::RegisterAudioTrackOutputStream(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

}  // namespace media
