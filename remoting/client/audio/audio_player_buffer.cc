// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/client/audio/audio_player_buffer.h"

#include <algorithm>
#include <string>

#include "base/logging.h"
#include "base/stl_util.h"

// If queue grows bigger than this we start dropping packets.
const int kMaxQueueLatencyMs = 150;

const int kFrameSizeMs = 10;

const int kMaxQueueLatencyFrames = kMaxQueueLatencyMs / kFrameSizeMs;

namespace remoting {

AudioFrameRequest::AudioFrameRequest(
    const void* samples,
    const void* context,
    const AudioBufferGetAudioFrameCallback& callback,
    const size_t bytes_needed)
    : samples_(samples),
      context_(context),
      callback_(callback),
      bytes_needed_(bytes_needed),
      bytes_extracted_(0) {}

AudioFrameRequest::~AudioFrameRequest() {}

AudioPlayerBuffer::AudioPlayerBuffer()
    : queued_bytes_(0), bytes_consumed_(0), weak_factory_(this) {
  ResetQueue();
}

AudioPlayerBuffer::~AudioPlayerBuffer() {
  //  base::AutoLock auto_lock(lock_);
  ResetQueue();
}

void AudioPlayerBuffer::AddAudioPacket(std::unique_ptr<AudioPacket> packet) {
  CHECK_EQ(1, packet->data_size());
  DCHECK_EQ(AudioPacket::ENCODING_RAW, packet->encoding());
  DCHECK_NE(AudioPacket::SAMPLING_RATE_INVALID, packet->sampling_rate());
  DCHECK_EQ(GetBufferedSamplingRate(), packet->sampling_rate());
  DCHECK_EQ(GetBufferedByesPerSample(),
            static_cast<int>(packet->bytes_per_sample()));
  DCHECK_EQ(GetBufferedChannels(), static_cast<int>(packet->channels()));
  DCHECK_EQ(packet->data(0).size() %
                (GetBufferedChannels() * GetBufferedByesPerSample()),
            0u);

  //  base::AutoLock auto_lock(lock_);

  // Push the new data to the back of the queue.
  queued_bytes_ += packet->data(0).size();
  queued_packets_.push_back(packet.release());

  // TODO(nicholss): MIGHT WANT TO JUST CALCULATE THIS ONCE.
  int max_buffer_size_ = kMaxQueueLatencyFrames * GetBytesPerFrame();

  // Trim off the front of the buffer if we have enqueued too many packets.
  while (queued_bytes_ > max_buffer_size_) {
    queued_bytes_ -= queued_packets_.front()->data(0).size() - bytes_consumed_;
    DCHECK_GE(queued_bytes_, 0);
    delete queued_packets_.front();
    queued_packets_.pop_front();
    bytes_consumed_ = 0;
  }

  // Attempt to process a FrameRequest now we have changed queued packets.
  ProcessFrameRequestQueue();
}

void AudioPlayerBuffer::Stop() {
  ResetQueue();
}

void AudioPlayerBuffer::AsyncGetAudioFrame(
    void* samples,
    uint32_t buffer_size,
    void* context,
    const AudioBufferGetAudioFrameCallback& callback) {
  // Create an AudioFrameRequest and enqueue it into the buffer.
  CHECK_EQ(buffer_size % GetBytesPerFrame(), 0u);
  AudioFrameRequest* audioFrameRequest =
      new AudioFrameRequest(samples, context, callback, buffer_size);
  queued_requests_.push_back(audioFrameRequest);
  ProcessFrameRequestQueue();
  return;
}
// static
void AudioPlayerBuffer::AudioPlayerCallback(void* samples,
                                            uint32_t buffer_size,
                                            void* data) {
  AudioPlayerBuffer* audio_buffer = static_cast<AudioPlayerBuffer*>(data);
  audio_buffer->GetAudioFrame(samples, buffer_size);
}

void AudioPlayerBuffer::ResetQueue() {
  //  lock_.AssertAcquired();
  STLDeleteElements(&queued_packets_);
  queued_bytes_ = 0;
  bytes_consumed_ = 0;
  STLDeleteElements(&queued_requests_);
}

uint32_t AudioPlayerBuffer::GetAudioFrame(void* samples, uint32_t buffer_size) {
  //  base::AutoLock auto_lock(lock_);

  const size_t bytes_needed = GetBytesPerFrame();

  // Make sure we don't overrun the buffer.
  CHECK_EQ(buffer_size, bytes_needed);

  char* next_sample = static_cast<char*>(samples);
  size_t bytes_extracted = 0;

  while (bytes_extracted < bytes_needed) {
    // Check if we've run out of samples for this packet.
    if (queued_packets_.empty()) {
      memset(next_sample, 0, bytes_needed - bytes_extracted);
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

// This is called when the Frame Request Queue or the Sample Queue is changed.
void AudioPlayerBuffer::ProcessFrameRequestQueue() {
  // TODO(nicholss): this function will attempt to copy data from the input
  // queue into a pending frame request as it can. If it is able to copy enough
  // data from the input queue into a request, it will call that request's
  // callback with the request context.

  // Get the active request if there is one.
  while (!queued_requests_.empty() && !queued_packets_.empty()) {
    //    base::AutoLock auto_lock(lock_);
    AudioFrameRequest* activeRequest = queued_requests_.front();

    // Copy any available data into the active request upto as much requested
    while (activeRequest->bytes_extracted_ < activeRequest->bytes_needed_ &&
           !queued_packets_.empty()) {
      char* next_sample = (char*)(activeRequest->samples_);

      const std::string& packet_data = queued_packets_.front()->data(0);
      size_t bytes_to_copy = std::min(
          packet_data.size() - bytes_consumed_,
          activeRequest->bytes_needed_ - activeRequest->bytes_extracted_);

      memcpy(next_sample, packet_data.data() + bytes_consumed_, bytes_to_copy);

      next_sample += bytes_to_copy;
      bytes_consumed_ += bytes_to_copy;
      activeRequest->bytes_extracted_ += bytes_to_copy;
      queued_bytes_ -= bytes_to_copy;
      DCHECK_GE(queued_bytes_, 0);
      // Pop off the packet if we've already consumed all its bytes.
      if (queued_packets_.front()->data(0).size() == bytes_consumed_) {
        delete queued_packets_.front();
        queued_packets_.pop_front();
        bytes_consumed_ = 0;
      }
    }

    // If this request is fulfilled, call the callback and pop it off the queue.
    if (activeRequest->bytes_extracted_ == activeRequest->bytes_needed_) {
      activeRequest->callback_.Run(activeRequest->context_,
                                   activeRequest->samples_);
      delete queued_requests_.front();
      queued_requests_.pop_front();
      activeRequest = nullptr;
    }
  }
}

uint32_t AudioPlayerBuffer::GetSamplesPerFrame() {
  return (GetBufferedSamplingRate() * kFrameSizeMs) /
         base::Time::kMillisecondsPerSecond;
}

AudioPacket::SamplingRate AudioPlayerBuffer::GetBufferedSamplingRate() {
  return AudioPacket::SAMPLING_RATE_48000;
}

AudioPacket::Channels AudioPlayerBuffer::GetBufferedChannels() {
  return AudioPacket::CHANNELS_STEREO;
}

AudioPacket::BytesPerSample AudioPlayerBuffer::GetBufferedByesPerSample() {
  return AudioPacket::BYTES_PER_SAMPLE_2;
}

uint32_t AudioPlayerBuffer::GetBytesPerFrame() {
  return GetBufferedChannels() * GetBufferedByesPerSample() *
         GetSamplesPerFrame();
}

base::WeakPtr<AudioConsumer> AudioPlayerBuffer::AudioConsumerAsWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

}  // namespace remoting
