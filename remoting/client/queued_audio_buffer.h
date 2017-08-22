// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_CLIENT_QUEUED_AUDIO_BUFFER_H_
#define REMOTING_CLIENT_QUEUED_AUDIO_BUFFER_H_

#include <stddef.h>
#include <stdint.h>

#include <list>

#include "base/macros.h"
#include "base/synchronization/lock.h"
#include "remoting/client/audio_buffer.h"
#include "remoting/proto/audio.pb.h"

namespace remoting {

class QueuedAudioBuffer : AudioBuffer {
 public:
  virtual ~QueuedAudioBuffer();

  // Deprecated, use ::Push(packet)
  void ProcessAudioPacket(std::unique_ptr<AudioPacket> packet);

  // Audio Buffer Interface:

  void Push(std::unique_ptr<AudioPacket> packet) override;
  uint32_t Pop(void* samples, uint32_t buffer_size) override;
  void Pop(void* samples,
           uint32_t buffer_size,
           void* context,
           const AudioBufferPopCallback& callback) override;
  void Reset() override;
  uint32_t Size() override;

  AudioPacket::SamplingRate GetSamplingRate() override;
  AudioPacket::Channels GetChannels() override;
  AudioPacket::BytesPerSample GetByesPerSample() override;

 protected:
  QueuedAudioBuffer();

  // Return the recommended number of samples to include in a frame.
  virtual uint32_t GetSamplesPerFrame() = 0;

  // Deprecated, use Play()
  virtual bool ResetAudioPlayer(AudioPacket::SamplingRate sampling_rate) = 0;

  // Function called by the browser when it needs more audio samples.
  //  static void AudioPlayerCallback(void* samples,
  //                                  uint32_t buffer_size,
  //                                  void* data);

 private:
  friend class AudioPlayerTest;

  typedef std::list<AudioPacket*> AudioPacketQueue;

  // Deprecated, use ::Reset()
  void ResetQueue();

  // Deprecated, use ::Play()
  void FillWithSamples(void* samples, uint32_t buffer_size);

  AudioPacket::SamplingRate sampling_rate_;
  AudioPacket::BytesPerSample bytes_per_sample_;
  AudioPacket::Channels channels_;

  bool start_failed_;

  // Protects |queued_packets_|, |queued_samples_ and |bytes_consumed_|. This is
  // necessary to prevent races, because Pepper will call the  callback on a
  // separate thread.
  base::Lock lock_;

  AudioPacketQueue queued_packets_;
  int queued_bytes_;

  // The number of bytes from |queued_packets_| that have been consumed.
  size_t bytes_consumed_;

  DISALLOW_COPY_AND_ASSIGN(QueuedAudioBuffer);
};

}  // namespace remoting

#endif  // REMOTING_CLIENT_QUEUED_AUDIO_BUFFER_H_
