// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_GPU_ANDROID_MEDIA_CODEC_VIDEO_DECODER_H_
#define MEDIA_GPU_ANDROID_MEDIA_CODEC_VIDEO_DECODER_H_

#include <deque>

#include "base/callback.h"
#include "base/callback_helpers.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/thread_checker.h"
#include "gpu/ipc/service/gpu_channel.h"
#include "media/base/android/media_codec_bridge_impl.h"
#include "media/base/android/media_codec_util.h"
#include "media/base/bind_to_current_loop.h"
#include "media/base/decoder_buffer.h"
#include "media/base/video_codecs.h"
#include "media/base/video_decoder.h"
#include "media/base/video_decoder_config.h"
#include "media/gpu/android/media_codec_wrapper.h"
#include "media/gpu/android/video_frame_factory.h"
#include "media/gpu/android_video_surface_chooser.h"
#include "media/gpu/avda_codec_allocator.h"
#include "media/gpu/ipc/service/media_gpu_channel_manager.h"
#include "media/gpu/media_gpu_export.h"

namespace media {

// This just turns a client interface into a callback.
// TODO(watk): Simplify the interface to AVDACodecAllocator.
class CodecAllocatorClient
    : public AVDACodecAllocatorClient,
      public base::SupportsWeakPtr<CodecAllocatorClient> {
 public:
  CodecAllocatorClient() : weak_factory(this) {}
  void OnCodecConfigured(
      std::unique_ptr<MediaCodecBridge> media_codec) override {
    codec_created_cb.Run(std::move(media_codec));
  }

  base::Callback<void(std::unique_ptr<MediaCodecBridge>)> codec_created_cb;
  base::WeakPtrFactory<CodecAllocatorClient> weak_factory;
};

enum class Status { kOk, kError };

struct PendingDecode {
  bool IsEOS() { return !buffer; }
  static PendingDecode Eos() {
    return PendingDecode{nullptr, VideoDecoder::DecodeCB()};
  }

  scoped_refptr<DecoderBuffer> buffer;
  VideoDecoder::DecodeCB decode_cb;
};


// The mutable state shared by MCVD and its flows.
// XXX: Should we make these MCVD members and pass them as raw pointers to
// flows? It's safe and makes the dependencies explicit. But it's also a bit
// unusual.
class MediaCodecVideoDecoder;
struct State {
  VideoDecoderConfig config;
  VideoDecoder::InitCB init_cb;
  VideoDecoder::OutputCB output_cb;
  base::Closure reset_cb;
  std::deque<PendingDecode> pending_decodes;

  AVDACodecAllocator* codec_allocator;
  CodecAllocatorClient codec_allocator_client;
  std::unique_ptr<MediaCodecWrapper> codec;
  std::unique_ptr<VideoFrameFactory> video_frame_factory;
  std::unique_ptr<AndroidVideoSurfaceChooser> surface_chooser;
  std::unique_ptr<AndroidOverlay> overlay;
  scoped_refptr<SurfaceTextureGLOwner> surface_texture;

  scoped_refptr<base::SingleThreadTaskRunner> gpu_task_runner;
  base::Callback<gpu::GpuCommandBufferStub*()> get_command_buffer_stub_cb;

  MediaCodecVideoDecoder* mcvd;
};

// Flows
// =====
//  - Flows are an abstraction for an asynchronous, cancelable, task.
//  - They're ephemeral, basically free to create, and composable.
//
// Interface
// =========
//  - Call Start() to start the task.
//  - The flow will post done_cb to the current thread when it's done.
//  - Call Cancel() to finish the flow early.
//  - After done_cb runs, or after Cancel() returns, it's possible to query the
//    status of the flow by calling status().
//
// Events
// ======
// Most events are handled by MCVD by updating its current state and canceling
// the current flow. This frees us from considering the cross product of current
// task and event.
//
// Events that have the lifetime of the flow are handled internally. E.g.,
// CodecCreated() can be handled internally by CreateCodecFlow; if the flow is
// canceled before the codec is created, it's safely dropped.
class Flow {
 public:
  Flow(State* state, base::Closure done_cb)
      : state_(state), done_cb_(std::move(done_cb)) {}

  virtual void Start() {}
  virtual void Cancel() {}

  // Returns the status of the flow. Only valid after Cancel() or done_cb runs.
  Status status() { return status_; }
  bool failed() { return status() != Status::kOk; }

  virtual void HandleDecode() {}

 protected:
  void set_status(Status status) { status_ = status; }

  void Finish(Status status) {
    status_ = status;
    base::ThreadTaskRunnerHandle::Get()->PostTask(FROM_HERE, done_cb_);
  }

  State* state_;
  base::Closure done_cb_;
  Status status_ = Status::kOk;
};

// An Android VideoDecoder that delegates to MediaCodec.
class MEDIA_GPU_EXPORT MediaCodecVideoDecoder : public VideoDecoder {
 public:
  MediaCodecVideoDecoder(
      scoped_refptr<base::SingleThreadTaskRunner> gpu_task_runner,
      base::Callback<gpu::GpuCommandBufferStub*()> get_command_buffer_stub_cb,
      AVDACodecAllocator* codec_allocator);
  ~MediaCodecVideoDecoder() override;

  // VideoDecoder implementation:
  std::string GetDisplayName() const override;
  void Initialize(const VideoDecoderConfig& config,
                  bool low_delay,
                  CdmContext* cdm_context,
                  const InitCB& init_cb,
                  const OutputCB& output_cb) override;
  void Decode(const scoped_refptr<DecoderBuffer>& buffer,
              const DecodeCB& decode_cb) override;
  void Reset(const base::Closure& reset_cb) override;
  bool NeedsBitstreamConversion() const override;
  bool CanReadWithoutStalling() const override;
  int GetMaxDecodeRequests() const override;

  void Destroy();
  void OnSurfaceTransition(std::unique_ptr<AndroidOverlay> overlay);
  void SurfaceDestroyed(AndroidOverlay* overlay);

 private:
  void StartNextFlow();
  void HandleFlowFailure();
  void CancelPendingDecodes(DecodeStatus decode_status);

  State state_;
  std::unique_ptr<Flow> flow_;

  Status status_;
  bool did_first_init_;
  bool destruction_pending_;

  base::WeakPtrFactory<MediaCodecVideoDecoder> flow_weak_factory_;
  base::WeakPtrFactory<MediaCodecVideoDecoder> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(MediaCodecVideoDecoder);
};

}  // namespace media

#endif  // MEDIA_GPU_ANDROID_MEDIA_CODEC_VIDEO_DECODER_H_
