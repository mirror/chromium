// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

#include "remoting/client/ios/audio_player_ios.h"

#include "remoting/client/ios/audio_consumer_proxy.h"
#include "remoting/client/ios/audio_player_buffer.h"
#include "remoting/client/ios/audio_player_wrapper.h"

#import <AudioToolbox/AudioToolbox.h>
#import <Foundation/Foundation.h>
#include "base/logging.h"

#include "base/memory/ptr_util.h"
#include "remoting/base/auto_thread.h"

namespace remoting {

// AudioQueue output queue callback.
static void AudioEngineOutputBufferCallback(void* instance,
                                            AudioQueueRef outAQ,
                                            AudioQueueBufferRef samples) {
  AudioPlayerIos* player = (AudioPlayerIos*)instance;
  player->Prime(samples, true);
}

std::unique_ptr<AudioPlayerIos> AudioPlayerIos::CreateAudioPlayer(
    scoped_refptr<AutoThreadTaskRunner> caller_thread_runner,
    scoped_refptr<AutoThreadTaskRunner> audio_thread_runner) {
  AudioPlayerBuffer* buffer = new AudioPlayerBuffer();

  // TODO(nicholss): might have to make a proxy fro the Supplier too.
  std::unique_ptr<AsyncAudioSupplier> supplier =
      (std::unique_ptr<AsyncAudioSupplier>)base::WrapUnique(buffer);

  std::unique_ptr<AudioConsumer> consumer =
      (std::unique_ptr<AudioConsumer>)base::WrapUnique(buffer);

  std::unique_ptr<AudioConsumer> consumer_proxy = AudioConsumerProxy::Create(
      caller_thread_runner, audio_thread_runner, std::move(consumer));

  return base::WrapUnique(new AudioPlayerIos(
      std::move(consumer_proxy), std::move(supplier), audio_thread_runner));
}

AudioPlayerIos::AudioPlayerIos(
    std::unique_ptr<protocol::AudioStub> audio_consumer,
    std::unique_ptr<AsyncAudioSupplier> audio_supplier,
    scoped_refptr<base::SingleThreadTaskRunner> audio_task_runner)
    : state_(UNALLOCATED),
      enqueued_frames_(0),
      audio_task_runner_(audio_task_runner) {
  audio_consumer_ = std::move(audio_consumer);
  audio_supplier_ = std::move(audio_supplier);
}

void AudioPlayerIos::StartPlayback() {
  OSStatus err;
  err = AudioQueueStart(output_queue_, nil);
  if (err) {
    LOG(FATAL) << "AudioQueueStart: " << err;
    state_ = UNKNOWN;
    return;  // false;
  }
  state_ = PLAYING;
  return;  // true;
}

void AudioPlayerIos::Start() {
  audio_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&AudioPlayerIos::StartOnAudioThread, base::Unretained(this)));
}

void AudioPlayerIos::StartOnAudioThread() {
  switch (state_) {
    case UNALLOCATED:
      GenerateOutputQueue((void*)this);
      GenerateOutputBuffers(audio_supplier_->GetBytesPerFrame());
      state_ = STOPPED;
    // Fall-through intended.
    case STOPPED:
      PrimeQueue();
    // Fall-through intended.
    case PRIMING:
      // Nothing to do.
      return;
    case PLAYING:
      // Nothing to do.
      return;
    case UNDERRUN:
      // Nothing to do.
      return;
    default:
      LOG(FATAL) << "Audio Player: Unknown State.";
  }
}

// Should only be called while |state_| is STOPPED
void AudioPlayerIos::PrimeQueue() {
  if (state_ != STOPPED) {
    return;
  }
  state_ = PRIMING;
  priming_ = kOutputBuffers;
  for (int i = 0; i < kOutputBuffers; i++) {
    Prime(output_buffers_[i], false);
  }
}

void AudioPlayerIos::AsyncGetAudioFrameCallback(const void* context,
                                                const void* samples) {
  AudioQueueBufferRef output_buffer = (AudioQueueBufferRef)context;
  output_buffer->mAudioDataByteSize = audio_supplier_->GetBytesPerFrame();
  Pump(output_buffer);
}

void AudioPlayerIos::Prime(AudioQueueBufferRef output_buffer,
                           bool was_dequeued) {
  if (was_dequeued) {
    --enqueued_frames_;
  }

  audio_supplier_->AsyncGetAudioFrame(
      reinterpret_cast<void*>(output_buffer->mAudioData),
      audio_supplier_->GetBytesPerFrame(),
      reinterpret_cast<void*>(output_buffer),
      base::Bind(&AudioPlayerIos::AsyncGetAudioFrameCallback,
                 base::Unretained(this)));

  if (state_ == PLAYING &&
      enqueued_frames_ == 0) {  // TODO(nicholss): remove magic number.
    state_ = UNDERRUN;
    StopOnAudioThread();
    // We have a bunch of pending requests so we are not stopped, but really in
    // PRIMING. Avoid re-priming the queue.
    state_ = PRIMING;
    priming_ = kOutputBuffers;
  }
}

void AudioPlayerIos::Pump(AudioQueueBufferRef output_buffer) {
  OSStatus err;

  err = AudioQueueEnqueueBuffer(output_queue_, output_buffer, 0, nil);

  if (err) {
    LOG(FATAL) << "AudioQueueEnqueueBuffer: " << err;
  } else {
    ++enqueued_frames_;
  }

  if (state_ == PRIMING) {
    --priming_;
    if (priming_ == 0) {
      audio_task_runner_->PostTask(
          FROM_HERE,
          base::Bind(&AudioPlayerIos::StartPlayback, base::Unretained(this)));
    }
  }
}

void AudioPlayerIos::Stop() {
  audio_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&AudioPlayerIos::StopOnAudioThread, base::Unretained(this)));
}

void AudioPlayerIos::StopOnAudioThread() {
  if (output_queue_) {
    AudioQueueStop(output_queue_, YES);
    //    AudioQueueFreeBuffer(outputQueue, outputBuffer); TODO(nicholss) look
    //    into freeing the buffer?
    //     AudioQueueDispose
  }
  state_ = STOPPED;
}

base::WeakPtr<protocol::AudioStub> AudioPlayerIos::GetAudioConsumer() {
  return (base::WeakPtr<protocol::AudioStub>)
      audio_consumer_->AudioConsumerAsWeakPtr();
}

bool AudioPlayerIos::ResetAudioPlayer() {
  if (state_ == PLAYING) {
    Stop();
  }
  Start();
  return true;
}

AudioStreamBasicDescription AudioPlayerIos::GenerateStreamFormat() {
  // Set up stream format fields
  // TODO(nicholss): does this all needs to be generated dynamicly?
  AudioStreamBasicDescription streamFormat;
  streamFormat.mSampleRate = 48000;
  streamFormat.mFormatID = kAudioFormatLinearPCM;
  streamFormat.mFormatFlags =
      kLinearPCMFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsPacked;
  streamFormat.mBitsPerChannel = 16;
  streamFormat.mChannelsPerFrame = 2;
  streamFormat.mBytesPerPacket = 2 * streamFormat.mChannelsPerFrame;
  streamFormat.mBytesPerFrame = 2 * streamFormat.mChannelsPerFrame;
  streamFormat.mFramesPerPacket = 1;
  streamFormat.mReserved = 0;
  return streamFormat;
}

bool AudioPlayerIos::GenerateOutputQueue(void* context) {
  // Set up stream format fields
  AudioStreamBasicDescription streamFormat = GenerateStreamFormat();
  OSStatus err;
  err = AudioQueueNewOutput(&streamFormat, AudioEngineOutputBufferCallback,
                            context, CFRunLoopGetCurrent(),
                            kCFRunLoopCommonModes, 0, &output_queue_);
  if (err) {
    LOG(FATAL) << "AudioQueueNewOutput: " << err;
    return false;
  }
  return true;
}

bool AudioPlayerIos::GenerateOutputBuffers(uint32_t bytesPerFrame) {
  OSStatus err;
  for (int i = 0; i < kOutputBuffers; i++) {
    err = AudioQueueAllocateBuffer(output_queue_, bytesPerFrame,
                                   &output_buffers_[i]);
    if (err) {
      LOG(FATAL) << "AudioQueueAllocateBuffer[" << i << "] : " << err;
      return false;
    }
  }
  return true;
}

AudioPlayerIos::~AudioPlayerIos() {
  // TODO(nicholss): Anything to clean up here? Maybe the allocated buffers and
  // the audio queue.
}

}  // namespace remoting
