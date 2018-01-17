// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_PUBLIC_MEDIA_AUDIO_POST_PROCESSOR2_SHLIB_H_
#define CHROMECAST_PUBLIC_MEDIA_AUDIO_POST_PROCESSOR2_SHLIB_H_

#include <string>
#include <vector>

#include "chromecast_export.h"
#include "volume_control.h"

// Creates an AudioPostProcessor.
// This is applicable only to Alsa CMA backend.
// Please refer to
// chromecast/media/cma/backend/post_processors/governor_shlib.cc
// as an example for new code, but OEM's implementations should not have any
// Chromium dependencies.
// Please refer to
// chromecast/media/cma/backend/post_processors/post_processor_wrapper.h for an
// example of how to port an existing AudioPostProcessor to AudioPostProcessor2
// Called from StreamMixerAlsa when shared objects are listed in
// /etc/cast_audio.json
// AudioPostProcessors are created on startup and only destroyed/reset
// if the output sample rate changes.
// libcast_FOOBAR_1.0.so should export a CREATE function as follows:
// AUDIO_POST_PROCESSOR2_SHLIB_CREATE_FUNC(FOOBAR) { }
#define AUDIO_POST_PROCESSOR2_SHLIB_CREATE_FUNC(type)                  \
  extern "C" CHROMECAST_EXPORT chromecast::media::AudioPostProcessor2* \
      AudioPostProcessor2##type##Create(const std::string& config,     \
                                        int channels_in)

namespace chromecast {
namespace media {

// Interface for AudioPostProcessors used for applying DSP in StreamMixerAlsa.
class AudioPostProcessor2 {
 public:
  // The maximum amount of data that will ever be processed in one call.
  static constexpr int kMaxAudioWriteTimeMilliseconds = 20;

  // Updates the sample rate of the processor.
  // Returns |false| if the processor cannot support |sample_rate|
  // Returning false will result in crashing cast_shell.
  virtual bool SetSampleRate(int sample_rate) = 0;

  // Returns the number of output channels. This value should never change after
  // construction.
  virtual int NumOutputChannels() = 0;

  // Processes audio frames from |data|.
  // Output buffer should be made available via GetOutputBuffer().
  // ProcessFrames may overwrite |data|, in which case GetOutputBuffer() should
  // return |data|.
  // |data| will always be 32-bit interleaved float with |channels_in| channels.
  // If |channels_out| is larger than |channels_in|, AudioPostProcessor2 must
  // own a buffer of length at least |channels_out| * frames;
  // |data| cannot be assumed to be larger than |chanels_in| * frames.
  // |frames| is the number of audio frames in data and is
  // always non-zero and less than or equal to |kMaxAudioWriteTimeMilliseconds|.
  // AudioPostProcessor must always provide |frames| frames of data back
  // (may output 0’s).
  // |system_volume| is the Cast Volume applied to the stream
  // (normalized to 0-1). It is the same as the cast volume set via alsa.
  // |volume_dbfs| is the actual attenuation in dBFS (-inf to 0), equivalent to
  // VolumeMap::VolumeToDbFS(|volume|).
  // AudioPostProcessor should assume that volume has already been applied.
  // Returns the current rendering delay of the filter in frames,
  // or negative if an error occurred during processing.
  // If an error occurred during processing, |data| should be unchanged.
  virtual int ProcessFrames(float* data,
                            int frames,
                            float system_volume,
                            float volume_dbfs) = 0;

  // Returns the data buffer in which the last output from ProcessFrames() was
  // stored.
  // This will never be called before ProcessFrames()
  // This data location should be valid at least until ProcessFrames() is called
  // again.
  // The data returned by GetOutputBuffer() should not be modified by this
  // instance until the next call to ProcessFrames().
  // If |channels_in| >= |channels_out|, this should return |data| from the
  // last call to ProcessFrames() (in-place processing).
  // If |channels_in| < |channels_out|, this PostProcessor is responsible for
  // allocating an output buffer and ensuring that the memory is valid until
  // destruction.
  virtual float* GetOutputBuffer() = 0;

  // Returns the number of frames of silence it will take for the
  // processor to come to rest.
  // This may be the actual number of frames stored,
  // or may be calculated from internal resonators or similar.
  // When inputs are paused, at least this |GetRingingTimeInFrames()| of
  // silence will be passed through the processor.
  // This is not expected to be real-time;
  // It should only change when SetSampleRate is called.
  virtual int GetRingingTimeInFrames() = 0;

  // Sends a message to the PostProcessor. Implementations are responsible
  // for the format and parsing of messages.
  // OEM's do not need to implement this method.
  virtual void UpdateParameters(const std::string& message) {}

  // Set content type to the PostProcessor so it could change processing
  // settings accordingly.
  virtual void SetContentType(AudioContentType content_type) {}

  // Called when device is playing as part of a stereo pair.
  // |channel| is the playout channel on this device (0 for left, 1 for right).
  // or -1 if the device is not part of a stereo pair.
  virtual void SetPlayoutChannel(int channel) {}

  virtual ~AudioPostProcessor2() = default;
};

}  // namespace media
}  // namespace chromecast

#endif  // CHROMECAST_PUBLIC_MEDIA_AUDIO_POST_PROCESSOR_SHLIB_H_
