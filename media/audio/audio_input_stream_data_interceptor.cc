// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/audio/audio_input_stream_data_interceptor.h"

namespace media {

  AudioInputStreamDataInterceptor::AudioInputStreamDataInterceptor(AudioInputStream* stream) : stream_(stream) {
    DCHECK(stream_);
  }

  AudioInputStreamDataInterceptor::~AudioInputStreamDataInterceptor() {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  }

  // Implementation of AudioInputStream.
  bool AudioInputStreamDataInterceptor::Open() {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    DCHECK(stream_);
    return stream_->Open();
  }
  
  void AudioInputStreamDataInterceptor::Start(media::AudioInputStream::AudioInputCallback* callback) {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    DCHECK(stream_);
    callback_ = callback;
    stream_->Start(this);
  }

  void AudioInputStreamDataInterceptor::Stop() {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    DCHECK(stream_);
    stream_->Stop();
    callback_ = nullptr;
  }
  
  void AudioInputStreamDataInterceptor::Close() {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    DCHECK(stream_);
    stream_->Close();
    delete this;
  }
  
  double AudioInputStreamDataInterceptor::GetMaxVolume() {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    DCHECK(stream_);
    return stream_->GetMaxVolume();
  }
  
  void AudioInputStreamDataInterceptor::SetVolume(double volume) {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    DCHECK(stream_);
    stream_->SetVolume(volume);
  }
  
  double AudioInputStreamDataInterceptor::GetVolume() {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    DCHECK(stream_);
    return stream_->GetVolume();
  }

  bool AudioInputStreamDataInterceptor::IsMuted() {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    DCHECK(stream_);
    return stream_->IsMuted();
  }

  void AudioInputStreamDataInterceptor::OnData(
                      const AudioBus* source,
                      base::TimeTicks capture_time,
                      double volume) {
    DCHECK(callback_);
    callback_->OnData(source, capture_time, volume);
}

  void AudioInputStreamDataInterceptor::OnError() {
    DCHECK(callback_);
    callback_->OnError();
  }

}  // namespace media
