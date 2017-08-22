// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/client/queued_audio_buffer.h"

#include <algorithm>

#include "base/logging.h"
#include "base/stl_util.h"
#include "remoting/client/audio_buffer.h"
#include "remoting/client/audio_player.h"

// The number of channels in the audio stream (only supporting stereo audio
// for now).
const int kChannels = 2;
const int kSampleSizeBytes = 2;

// If queue grows bigger than 150ms we start dropping packets.
const int kMaxQueueLatencyMs = 150;

namespace remoting {

QueuedAudioBuffer::QueuedAudioBuffer()
    : sampling_rate_(AudioPacket::SAMPLING_RATE_INVALID),
      bytes_per_sample_(AudioPacket::BYTES_PER_SAMPLE_INVALID),
      channels_(AudioPacket::CHANNELS_INVALID),
      start_failed_(false),
      queued_bytes_(0),
      bytes_consumed_(0) {}

QueuedAudioBuffer::~QueuedAudioBuffer() {
  base::AutoLock auto_lock(lock_);
  ResetQueue();
}

void QueuedAudioBuffer::Push(std::unique_ptr<AudioPacket> packet) {
  CHECK_EQ(1, packet->data_size());
  DCHECK_EQ(AudioPacket::ENCODING_RAW, packet->encoding());
  DCHECK_NE(AudioPacket::SAMPLING_RATE_INVALID, packet->sampling_rate());
  DCHECK_EQ(kSampleSizeBytes, packet->bytes_per_sample());
  DCHECK_EQ(static_cast<int>(kChannels), packet->channels());
  DCHECK_EQ(packet->data(0).size() % (kChannels * kSampleSizeBytes), 0u);

  // No-op if the player won't start.
  if (start_failed_) {
    return;
  }

  // Start the Pepper audio player if this is the first packet.
  if (sampling_rate_ != packet->sampling_rate() ||
      bytes_per_sample_ != packet->bytes_per_sample() ||
      channels_ != packet->channels()) {
    // Drop all packets currently in the queue, since they are sampled at the
    // wrong rate.
    {
      base::AutoLock auto_lock(lock_);
      ResetQueue();
    }

    sampling_rate_ = packet->sampling_rate();
    bytes_per_sample_ = packet->bytes_per_sample();
    channels_ = packet->channels();
    bool success = ResetAudioPlayer(sampling_rate_);
    if (!success) {
      start_failed_ = true;
      return;
    }
  }

  base::AutoLock auto_lock(lock_);

  queued_bytes_ += packet->data(0).size();
  queued_packets_.push_back(packet.release());

  int max_buffer_size_ = kMaxQueueLatencyMs * sampling_rate_ *
                         kSampleSizeBytes * kChannels /
                         base::Time::kMillisecondsPerSecond;
  while (queued_bytes_ > max_buffer_size_) {
    queued_bytes_ -= queued_packets_.front()->data(0).size() - bytes_consumed_;
    DCHECK_GE(queued_bytes_, 0);
    delete queued_packets_.front();
    queued_packets_.pop_front();
    bytes_consumed_ = 0;
    LOG(ERROR) << "Dropping packets from audio queue...";
  }
}

uint32_t QueuedAudioBuffer::Pop(void* samples, uint32_t buffer_size) {
  base::AutoLock auto_lock(lock_);

  const size_t bytes_needed =
      kChannels * kSampleSizeBytes * GetSamplesPerFrame();

  // Make sure we don't overrun the buffer.
  CHECK_EQ(buffer_size, bytes_needed);

  char* next_sample = static_cast<char*>(samples);
  size_t bytes_extracted = 0;

  while (bytes_extracted < bytes_needed) {
    // Check if we've run out of samples for this packet.
    if (queued_packets_.empty()) {
      memset(next_sample, 0, bytes_needed - bytes_extracted);
      LOG(ERROR) << "Filling audio queue with empty data... "
                 << (bytes_needed - bytes_extracted);
      return 0;
    }

    // Pop off the packet if we've already consumed all its bytes.
    if (queued_packets_.front()->data(0).size() == bytes_consumed_) {
      delete queued_packets_.front();
      queued_packets_.pop_front();
      bytes_consumed_ = 0;
      continue;
    }

    const std::string& packet_data = queued_packets_.front()->data(0);
    size_t bytes_to_copy = std::min(packet_data.size() - bytes_consumed_,
                                    bytes_needed - bytes_extracted);
    memcpy(next_sample, packet_data.data() + bytes_consumed_, bytes_to_copy);

    next_sample += bytes_to_copy;
    bytes_consumed_ += bytes_to_copy;
    bytes_extracted += bytes_to_copy;
    queued_bytes_ -= bytes_to_copy;
    DCHECK_GE(queued_bytes_, 0);
  }
  return bytes_extracted;
}

void QueuedAudioBuffer::Pop(void* samples,
                            uint32_t buffer_size,
                            void* context,
                            const AudioBufferPopCallback& callback) {
  // not implemented.
}

void QueuedAudioBuffer::Reset() {
  base::AutoLock auto_lock(lock_);
  ResetQueue();
}

uint32_t QueuedAudioBuffer::Size() {
  return queued_bytes_;
}

AudioPacket::SamplingRate QueuedAudioBuffer::GetSamplingRate() {
  return sampling_rate_;
}
AudioPacket::Channels QueuedAudioBuffer::GetChannels() {
  return channels_;
}

AudioPacket::BytesPerSample QueuedAudioBuffer::GetByesPerSample() {
  return bytes_per_sample_;
}

void QueuedAudioBuffer::ProcessAudioPacket(
    std::unique_ptr<AudioPacket> packet) {
  Push(std::move(packet));
}

/*
// static
void QueuedAudioBuffer::AudioPlayerCallback(void* samples,
                                          uint32_t buffer_size,
                                          void* data) {
  QueuedAudioBuffer* audio_player = static_cast<QueuedAudioBuffer*>(data);
  audio_player->Pop(samples, buffer_size);
}
*/

void QueuedAudioBuffer::ResetQueue() {
  lock_.AssertAcquired();
  queued_packets_.clear();
  queued_bytes_ = 0;
  bytes_consumed_ = 0;
}

void QueuedAudioBuffer::FillWithSamples(void* samples, uint32_t buffer_size) {
  Pop(samples, buffer_size);
}

}  // namespace remoting
