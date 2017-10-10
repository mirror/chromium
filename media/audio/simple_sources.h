// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_AUDIO_SIMPLE_SOURCES_H_
#define MEDIA_AUDIO_SIMPLE_SOURCES_H_

#include <stdint.h>

#include <memory>

#include "base/files/file_path.h"
#include "base/synchronization/lock.h"
#include "base/time/time.h"
#include "media/audio/audio_io.h"
#include "media/base/audio_converter.h"
#include "media/base/seekable_buffer.h"

namespace media {

class WavAudioHandler;

// An audio source that produces a pure sinusoidal tone.
class MEDIA_EXPORT SineWaveAudioSource
    : public AudioOutputStream::AudioSourceCallback {
 public:
  // |channels| is the number of audio channels, |freq| is the frequency in
  // hertz and it has to be less than half of the sampling frequency
  // |sample_rate| or else you will get aliasing.
  SineWaveAudioSource(int channels, double freq, double sample_rate);
  ~SineWaveAudioSource() override;

  // Return up to |cap| samples of data via OnMoreData().  Use Reset() to
  // allow more data to be served.
  void CapSamples(int cap);
  void Reset();

  // Implementation of AudioSourceCallback.
  int OnMoreData(base::TimeDelta delay,
                 base::TimeTicks timestamp,
                 int prior_frames_skipped,
                 AudioBus* dest) override;
  void OnError() override;

  // The number of OnMoreData() and OnError() calls respectively.
  int callbacks() { return callbacks_; }
  int errors() { return errors_; }

  base::TimeDelta stream_time() {
    return base::TimeDelta::FromSecondsD(static_cast<double>(stream_pos_) /
                                         sample_rate_);
  }

  void set_expected_period_size(int expected_period_size) {
    expected_period_size_ = expected_period_size;
  }

 protected:
  const int channels_;
  const double f_;
  const double sample_rate_;

  int expected_period_size_ = -1;

  int stream_pos_ = 0;
  int cap_ = 0;
  int callbacks_ = 0;
  int errors_ = 0;
  base::Lock time_lock_;
};

class MEDIA_EXPORT FileSource : public AudioOutputStream::AudioSourceCallback,
                                public AudioConverter::InputCallback {
 public:
  FileSource(const AudioParameters& params,
             const base::FilePath& path_to_wav_file,
             bool loop);
  ~FileSource() override;

  // Implementation of AudioSourceCallback.
  int OnMoreData(base::TimeDelta delay,
                 base::TimeTicks delay_timestamp,
                 int prior_frames_skipped,
                 AudioBus* dest) override;
  void OnError() override;

 private:
  AudioParameters params_;
  base::FilePath path_to_wav_file_;

  // The WAV data at |path_to_wav_file_| is read into memory and kept here.
  // This memory needs to survive for the lifetime of |wav_audio_handler_|,
  // so declare it first. Do not access this member directly.
  std::unique_ptr<char[]> raw_wav_data_;

  std::unique_ptr<WavAudioHandler> wav_audio_handler_;
  std::unique_ptr<AudioConverter> file_audio_converter_;
  int wav_file_read_pos_;
  bool load_failed_;
  bool looping_;

  // Provides audio data from wav_audio_handler_ into the file audio converter.
  double ProvideInput(AudioBus* audio_bus, uint32_t frames_delayed) override;

  // Loads the wav file on the first OnMoreData invocation.
  void LoadWavFile(const base::FilePath& path_to_wav_file);

  // Rewinds the player to the start of the loaded wav file.
  void Rewind();
};

class BeepingSource : public AudioOutputStream::AudioSourceCallback {
 public:
  explicit BeepingSource(const AudioParameters& params);
  ~BeepingSource() override;

  // Implementation of AudioSourceCallback.
  int OnMoreData(base::TimeDelta delay,
                 base::TimeTicks delay_timestamp,
                 int prior_frames_skipped,
                 AudioBus* dest) override;
  void OnError() override;

  static void BeepOnce();
 private:
  int buffer_size_;
  std::unique_ptr<uint8_t[]> buffer_;
  AudioParameters params_;
  base::TimeTicks last_callback_time_;
  base::TimeDelta interval_from_last_beep_;
  int beep_duration_in_buffers_;
  int beep_generated_in_buffers_;
  int beep_period_in_frames_;
};

}  // namespace media

#endif  // MEDIA_AUDIO_SIMPLE_SOURCES_H_
