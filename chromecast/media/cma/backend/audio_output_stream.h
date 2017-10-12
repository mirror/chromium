// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_MEDIA_CMA_BACKEND_AUDIO_OUTPUT_STREAM_H_
#define CHROMECAST_MEDIA_CMA_BACKEND_AUDIO_OUTPUT_STREAM_H_

#include <memory>
#include <string>

#include "base/time/time.h"
#include "chromecast/public/media/media_pipeline_backend.h"

namespace chromecast {
namespace media {

const int kInvalidSampleRate = 0;

// Interface for output audio stream. Used by the mixer to play mixed stream.
class AudioOutputStream {
 public:
  // Creates default AudioOutputStream.
  static std::unique_ptr<AudioOutputStream> Create();

  virtual ~AudioOutputStream() {}

  // Sets device name to open. Must be called before Start(), otherwise default
  // device will be opened. The format of the name is platform-specific.
  virtual void SetDeviceName(const std::string& device_name) = 0;

  // Returns true if the sample rate is fixed.
  virtual bool IsFixedSampleRate() = 0;

  // Start the stream. Caller must call GetSampleRate() to get the actual sample
  // rate selected for the stream. It may be different from
  // |requested_sample_rate|, e.g. if IsFixedSampleRate() is true, or the device
  // doesn't support |requested_sample_rate|.
  virtual bool Start(int requested_sample_rate, int channels) = 0;

  // Returns current sample rate.
  virtual int GetSampleRate() = 0;

  // Returns current rendering delay for the stream.
  virtual MediaPipelineBackend::AudioDecoder::RenderingDelay
  GetRenderingDelay() = 0;

  // Return how much time is left to call Write() to prevent buffer underrun.
  virtual bool GetTimeUntilUnderrun(base::TimeDelta* result) = 0;

  // |data_size| is size of |data|. Should be divided by number
  // of channels to get number of frames.
  virtual bool Write(const float* data,
                     int data_size,
                     bool* out_playback_interrupted) = 0;

  // Stops the stream. After being stopped the stream can be restarted by
  // calling Start().
  virtual void Stop() = 0;
};

}  // namespace media
}  // namespace chromecast

#endif  // CHROMECAST_MEDIA_CMA_BACKEND_AUDIO_OUTPUT_STREAM_H_
