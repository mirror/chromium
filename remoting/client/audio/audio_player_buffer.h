// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_IOS_AUDIO_AUDIO_PLAYER_BUFFER_H_
#define REMOTING_IOS_AUDIO_AUDIO_PLAYER_BUFFER_H_

#include <cstdint>

#include <list>
#include <memory>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/synchronization/lock.h"
#include "remoting/client/audio/async_audio_frame_supplier.h"
#include "remoting/client/audio/audio_stream_consumer.h"
#include "remoting/proto/audio.pb.h"

namespace remoting {

struct AudioFrameRequest {
  const void* samples_;
  base::Closure callback_;
  const size_t bytes_needed_;
  size_t bytes_extracted_;

  explicit AudioFrameRequest(const void* samples,
                             const base::Closure& callback,
                             const size_t bytes_needed);
  ~AudioFrameRequest();
};

// This class consumes the decoded audio stream, buffers it into frames, and
// supplies the player with constant audio frames as they are ready.
// Audio Pipeline Context:
// Stream -> Decode -> [Stream Consumer -> Buffer -> Frame Provider] -> Play
class AudioPlayerBuffer : public AudioStreamConsumer,
                          public AsyncAudioFrameSupplier {
 public:
  // The number of channels in the audio stream (only supporting stereo audio
  // for now).
  static const int kChannels = 2;
  static const int kSampleSizeBytes = 2;

  AudioPlayerBuffer();
  ~AudioPlayerBuffer() override;

  void Stop();
  void Reset();

  // Audio Stream Consumer
  void AddAudioPacket(std::unique_ptr<AudioPacket> packet) override;
  base::WeakPtr<AudioStreamConsumer> AudioConsumerAsWeakPtr() override;

  // Async Audio Frame Supplier
  void AsyncGetAudioFrame(uint32_t buffer_size,
                          void* buffer,
                          const base::Closure& callback) override;

  // Audio Frame Supplier
  uint32_t GetAudioFrame(uint32_t buffer_size, void* buffer) override;
  AudioPacket::SamplingRate buffered_sampling_rate() const override;
  AudioPacket::Channels buffered_channels() const override;
  AudioPacket::BytesPerSample buffered_byes_per_sample() const override;
  uint32_t bytes_per_frame() const override;

  // Return the recommended number of samples to include in a frame.
  uint32_t samples_per_frame() const;

 protected:
  // Function called by the browser when it needs more audio samples.
  static void AudioPlayerCallback(void* buffer,
                                  uint32_t buffer_size,
                                  void* data);

 private:
  friend class AudioPlayerBufferTest;

  typedef std::list<AudioPacket*> AudioPacketQueue;
  typedef std::list<AudioFrameRequest*> AudioFrameRequestQueue;

  void ResetQueue();

  void ProcessFrameRequestQueue();

  // Protects |queued_packets_|, |queued_requests_|, |queued_samples_ and
  // |bytes_consumed_|. This is necessary to prevent races, because Audio
  // Player will call the  callback on a separate thread.
  base::Lock lock_;

  AudioPacketQueue queued_packets_;
  int queued_bytes_;

  // The number of bytes from |queued_packets_| that have been consumed.
  size_t bytes_consumed_;

  AudioFrameRequestQueue queued_requests_;

  base::WeakPtrFactory<AudioPlayerBuffer> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(AudioPlayerBuffer);
};

}  // namespace remoting

#endif  // REMOTING_IOS_AUDIO_AUDIO_PLAYER_BUFFER_H_
