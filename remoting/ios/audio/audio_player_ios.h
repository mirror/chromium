// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_IOS_AUDIO_AUDIO_PLAYER_IOS_H_
#define REMOTING_IOS_AUDIO_AUDIO_PLAYER_IOS_H_

#include <memory>
#include <string>

#include "base/bind.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "remoting/base/auto_thread.h"

#import <AudioToolbox/AudioToolbox.h>

#include "remoting/client/async_audio_supplier.h"
#include "remoting/client/audio_consumer.h"

namespace remoting {

const int kOutputBuffers = 8;

typedef enum {
  UNALLOCATED,
  STOPPED,
  PRIMING,
  PLAYING,
  UNDERRUN,
  UNKNOWN
} AudioPlayerState;

/*
  For iOS, the audio subsystem uses a multi-buffer queue to produce smooth
  audio playback. To allow this to work with remoting, we need to add a buffer
  that is capable of enqueuing exactly the requested amount of data
  asynchronously then calling us back and we will push it into the iOS audio
  queue.
*/
class AudioPlayerIos {
 public:
  static std::unique_ptr<AudioPlayerIos> CreateAudioPlayer(
      scoped_refptr<AutoThreadTaskRunner> calling_thread_runner,
      scoped_refptr<AutoThreadTaskRunner> audio_thread_runner);

  base::WeakPtr<protocol::AudioStub> GetAudioConsumer();

  void Start();
  void Stop();

  void Prime(AudioQueueBufferRef output_buffer, bool was_dequeued);
  void Pump(AudioQueueBufferRef output_buffer);

  ~AudioPlayerIos();

 private:
  AudioPlayerIos(std::unique_ptr<protocol::AudioStub> audio_consumer,
                 std::unique_ptr<AsyncAudioSupplier> audio_supplier,
                 scoped_refptr<base::SingleThreadTaskRunner> audio_task_runner);

  void StartOnAudioThread();
  void StopOnAudioThread();

  void PrimeQueue();
  bool ResetAudioPlayer();
  void StartPlayback();

  void AsyncGetAudioFrameCallback(const void* context, const void* samples);

  bool GenerateOutputBuffers(uint32_t bytesPerFrame);
  bool GenerateOutputQueue(void* context);
  AudioStreamBasicDescription GenerateStreamFormat();

  AudioPlayerState state_;
  int priming_;
  AudioQueueRef output_queue_;
  AudioQueueBufferRef output_buffers_[kOutputBuffers];

  std::unique_ptr<protocol::AudioStub> audio_consumer_;
  std::unique_ptr<AsyncAudioSupplier> audio_supplier_;

  uint32_t enqueued_frames_;

  // Task runner on which |host_window_| is running.
  scoped_refptr<base::SingleThreadTaskRunner> audio_task_runner_;
};

}  // namespace remoting

#endif  // REMOTING_IOS_AUDIO_AUDIO_PLAYER_IOS_H_
