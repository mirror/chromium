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

struct State {
  VideoDecoderConfig config;
  VideoDecoder::InitCB init_cb;
  VideoDecoder::OutputCB output_cb;
  std::unique_ptr<MediaCodecWrapper> codec;
  std::unique_ptr<VideoFrameFactory> video_frame_factory;
  scoped_refptr<base::SingleThreadTaskRunner> gpu_task_runner;
  std::deque<PendingDecode> pending_decodes;
  base::Closure reset_cb;
  scoped_refptr<SurfaceTextureGLOwner> surface_texture;
  std::unique_ptr<AndroidOverlay> overlay;

  // XXX: Replace with !pending_decodes_.empty()
  bool has_pending_decodes_ = false;
};

// Flow principles
// ===============
//  - An abstraction for an asynchronous, cancelable, task.
//  - Ephemeral, cheap to create.
//  - Composable.
//  - Flows have a single threaded interface, but may do whatever they want
//    internally.
//  - If a flow finishes, its post conditions are satisfied.
//  - Flows are "resumable". You can cancel flow A and later start
//    a new flow A and it will continue from wherever it left off. The
//    "progress" isn't saved in the Flow, it's saved in the shared state struct.
//
// Rough Flow API
// ==============
//  - Call Start() once only.
//  - The flow posts done_cb to the current thread when it's done.
//  - Call Cancel() once to finish the flow early. The Flow won't post done_cb.
//    The flow must cancel all ongoing work and be safe to destruct when
//    Cancel() returns. The reason this has to be synchronous is so that we can
//    handle surfaceDestroyed() by canceling the current flow and releasing
//    the codec. We don't want to have to make all flows aware that the codec
//    might be released at any time.
//  - The caller of Cancel() must remember that done_cb might have already been
//    posted before Cancel() was called so done_cb should be bound to a weak_ptr
//    that can be invalidated before calling cancel.
//  - TODO(watk): Strongly consider Cancel() not posting done_cb. Things would
//    The point of Cancel() being synchronous and not calling
//    done_cb was that it seemed like the alternative would be boilerplatey for
//    flows that have subflows. In Cancel() they would have to Cancel() their
//    subflows and arrange for the parent done_cb to be called after all the
//    child done_cbs.
//    On the other hand, it complicates error reporting because you can't just
//    always receive the flow result in done_cb. And it requires the Flow client
//    to have to explicitly invalidate weak_ptrs. So it might actually be better
//    to go to that model.
//  - TODO(watk): The base Flow is a noop implementation. May not be necessary.
//
// Flow events
// ===========
// Some events are handled at the MCVD level (e.g., surfaceDestroyed()) becuase
// we don't want all flows to have code to handle those events; it's easier
// to cancel them.
//
// Other events can and should be handled by Flows. Whether all of these
// actually route through MCVD or not is an implementation detail.
//
// For events that arrive at MCVD and get passed to flows it's probably easiest
// to use a virtual function for each of them, and flows override the ones they
// care about.
//
// This is a list of all conceptual inputs to MCVD:
//  - Initialize()
//  - Decode()
//  - Reset()
//  - Destroy()
//  - OnSurfaceAvailable()
//  - OnSurfaceDestroyed()
//  - OnKeyAdded()
//  - OnCodecCreated()
//  - Codec buffer released
class Flow {
 public:
  Flow(State* state, base::Closure done_cb)
      : state_(state), done_cb_(std::move(done_cb)) {}

  virtual void Start() {}
  virtual void Cancel() {}

  virtual void DecodeEvent() {}

  void set_status(Status status) { status_ = status; }
  Status status() { return status_; }
  bool failed() { return status_ != Status::kOk; }

  void PostDoneCb() {
    base::ThreadTaskRunnerHandle::Get()->PostTask(FROM_HERE, done_cb_);
  }

 protected:
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
  bool InitPending();
  void HandleFlowFailure();

 private:
  void StartNextFlow();

  State state_;
  std::unique_ptr<Flow> flow_;

  // XXX: Initialize members in constructor.
  InitCB init_cb_;
  bool did_first_init_ = false;
  bool reset_pending_ = false;
  base::Closure reset_cb_;
  bool destruction_pending_ = false;
  Status status_ = Status::kOk;
  bool deferred_init_pending_ = false;

  scoped_refptr<base::SingleThreadTaskRunner> gpu_task_runner_;
  base::Callback<gpu::GpuCommandBufferStub*()> get_command_buffer_stub_cb_;
  AVDACodecAllocator* codec_allocator_;
  CodecAllocatorClient codec_allocator_client_;
  std::unique_ptr<AndroidVideoSurfaceChooser> surface_chooser_;

  base::WeakPtrFactory<MediaCodecVideoDecoder> flow_weak_factory_;
  base::WeakPtrFactory<MediaCodecVideoDecoder> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(MediaCodecVideoDecoder);
};

}  // namespace media

#endif  // MEDIA_GPU_ANDROID_MEDIA_CODEC_VIDEO_DECODER_H_
