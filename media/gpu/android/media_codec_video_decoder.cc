// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/gpu/android/media_codec_video_decoder.h"

#include "base/callback.h"
#include "base/callback_helpers.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/timer/elapsed_timer.h"
#include "gpu/ipc/service/gpu_command_buffer_stub.h"
#include "media/base/android/media_codec_bridge_impl.h"
#include "media/base/android/media_codec_util.h"
#include "media/base/bind_to_current_loop.h"
#include "media/base/surface_manager.h"
#include "media/base/video_codecs.h"
#include "media/base/video_decoder_config.h"
#include "media/gpu/android/coalescing_timer.h"
#include "media/gpu/android_video_surface_chooser_impl.h"
#include "media/gpu/avda_codec_allocator.h"

namespace media {
namespace {

const constexpr base::TimeDelta kCodecIdleTimeout =
    base::TimeDelta::FromSeconds(1);

// Don't use MediaCodec's internal software decoders when we have more secure
// and up to date versions in the renderer process.
bool IsMediaCodecSoftwareDecodingForbidden(const VideoDecoderConfig& config) {
  return !config.is_encrypted() &&
         (config.codec() == kCodecVP8 || config.codec() == kCodecVP9);
}

bool ConfigSupported(const VideoDecoderConfig& config) {
  // Don't support larger than 4k because it won't perform well on many devices.
  const auto size = config.coded_size();
  if (size.width() > 3840 || size.height() > 2160)
    return false;

  // Only use MediaCodec for VP8 or VP9 if it's likely backed by hardware or if
  // the stream is encrypted.
  const auto codec = config.codec();
  if (IsMediaCodecSoftwareDecodingForbidden(config) &&
      MediaCodecUtil::IsKnownUnaccelerated(codec,
                                           MediaCodecDirection::DECODER)) {
    DVLOG(1) << "Unable to create a HW backed MediaCodec for "
             << GetCodecName(codec);
    return false;
  }

  switch (codec) {
    case kCodecVP8:
    case kCodecVP9: {
      if ((codec == kCodecVP8 && !MediaCodecUtil::IsVp8DecoderAvailable()) ||
          (codec == kCodecVP9 && !MediaCodecUtil::IsVp9DecoderAvailable())) {
        return false;
      }

      // There's no fallback for encrypted content so we support all sizes.
      if (config.is_encrypted())
        return true;

      // Below 360p there's little to no power benefit to using MediaCodec over
      // libvpx so we prefer to use our newer version of libvpx, sandboxed in
      // the renderer.
      if (size.width() < 480 || size.height() < 360)
        return false;

      return true;
    }
    case kCodecH264:
      return true;
#if BUILDFLAG(ENABLE_HEVC_DEMUXING)
    case kCodecHEVC:
      return true;
#endif
    default:
      return false;
  }
}

}  // namespace

class InitVideoFrameFactoryFlow : public Flow {
 public:
  InitVideoFrameFactoryFlow(
      State* state,
      base::Closure done_cb,
      scoped_refptr<base::SingleThreadTaskRunner> gpu_task_runner,
      base::Callback<gpu::GpuCommandBufferStub*()> get_command_buffer_stub_cb)
      : Flow(state, done_cb),
        gpu_task_runner_(gpu_task_runner),
        get_command_buffer_stub_cb_(get_command_buffer_stub_cb),
        weak_factory_(this) {}

  void Start() override {
    DVLOG(1) << "InitVideoFrameFactoryFlow::" << __func__;
    if (!state_->video_frame_factory) {
      factory_.reset(new VideoFrameFactory(gpu_task_runner_));
      factory_->Initialize(
          get_command_buffer_stub_cb_,
          base::Bind(&InitVideoFrameFactoryFlow::FactoryInitialized,
                     weak_factory_.GetWeakPtr()));
    } else {
      PostDoneCb();
    }
  }

  void FactoryInitialized(
      scoped_refptr<SurfaceTextureGLOwner> surface_texture) {
    DVLOG(1) << "InitVideoFrameFactoryFlow::" << __func__;
    if (surface_texture) {
      state_->surface_texture = surface_texture;
      state_->video_frame_factory = std::move(factory_);
    } else {
      set_status(Status::kError);
    }
    PostDoneCb();
  }

  void Cancel() override {
    DVLOG(1) << "InitVideoFrameFactoryFlow::" << __func__;
    weak_factory_.InvalidateWeakPtrs();
  }

  std::unique_ptr<VideoFrameFactory> factory_;
  scoped_refptr<base::SingleThreadTaskRunner> gpu_task_runner_;
  base::Callback<gpu::GpuCommandBufferStub*()> get_command_buffer_stub_cb_;
  base::WeakPtrFactory<InitVideoFrameFactoryFlow> weak_factory_;
};

class ReleaseCodecFlow : public Flow {
  using Flow::Flow;
};

class CreateCodecFlow : public Flow {
 public:
  CreateCodecFlow(State* state,
                  base::Closure done_cb,
                  AVDACodecAllocator* codec_allocator)
      : Flow(state, done_cb),
        codec_allocator_(codec_allocator),
        weak_factory_(this) {}

  void Start() override {
    DVLOG(1) << "CreateCodecFlow::" << __func__;
    // If we already have a codec with the right config we're done.
    if (state_->codec && state_->config.Matches(state_->codec->config())) {
      PostInitAndDoneCbs();
      return;
    }

    // Release current codec if necessary.
    // If we haven't acquired state_->config->cdm_id(): start acquiring it.
    // If we haven't acquired state_->config->surface_id(): start acquiring it.
    StartCodecCreation();
  }

  void Cancel() override {
    DVLOG(1) << "CreateCodecFlow::" << __func__;
    client_.weak_factory.InvalidateWeakPtrs();
  }

  void PostInitAndDoneCbs() {
    if (state_->init_cb)
      base::ResetAndReturn(&state_->init_cb).Run(status() == Status::kOk);
    PostDoneCb();
  }

  void StartCodecCreation() {
    DVLOG(1) << "CreateCodecFlow::" << __func__;
    scoped_refptr<CodecConfig> codec_config = new CodecConfig();
    auto& config = state_->config;
    codec_config->codec = config.codec();
    bundle_ = new AVDASurfaceBundle(state_->surface_texture);
    codec_config->surface_bundle = bundle_;
    codec_config->initial_expected_coded_size = config.coded_size();
    // TODO(watk): Put back sps & pps parsing.
    client_.codec_created_cb =
        base::Bind(&CreateCodecFlow::CodecCreated, weak_factory_.GetWeakPtr());
    codec_allocator_->CreateMediaCodecAsync(client_.weak_factory.GetWeakPtr(),
                                            codec_config);
  }

  void CodecCreated(std::unique_ptr<MediaCodecBridge> codec) {
    DVLOG(1) << "CreateCodecFlow::" << __func__;
    if (codec) {
      // TODO(watk): Codec allocator should return a CodecWrapper so we don't
      // have to set this manually.
      state_->codec.reset(new MediaCodecWrapper(std::move(codec)));
      state_->codec->set_surface_bundle(bundle_);
    } else {
      set_status(Status::kError);
    }
    PostInitAndDoneCbs();
  }

  AVDACodecAllocator* codec_allocator_;
  CodecAllocatorClient client_;
  scoped_refptr<AVDASurfaceBundle> bundle_;
  base::WeakPtrFactory<CreateCodecFlow> weak_factory_;
};

// DestructFlow
//  * Unhook GLImages
//  * Release decoder
//  * Release the MC & surface
//
// The only thing that Cancels() DestructFlow in practice is surfaceDestroyed.
class DestructFlow : public Flow {
 public:
  DestructFlow(State* state,
               base::Closure done_cb,
               MediaCodecVideoDecoder* mcvd_)
      : Flow(state, done_cb), mcvd_(mcvd_), weak_factory_(this) {}

  void Start() override {
    DVLOG(1) << "DestructFlow::" << __func__;
    // UnhookVideoFrameImages();

    if (!state_->codec || !state_->codec->NeedsDrainingBeforeRelease()) {
      CodecReleased();
      return;
    }

    state_->pending_decodes.clear();
    if (!state_->codec->EosSubmitted())
      state_->pending_decodes.push_back(PendingDecode::Eos());

    releaser_ = base::MakeUnique<ReleaseCodecFlow>(
        state_,
        base::Bind(&DestructFlow::CodecReleased, weak_factory_.GetWeakPtr()));
    releaser_->Start();
  }

  // XXX: annoying that all flows with subflows have to do essentially the same
  // thing. Potentially solvable
  void Cancel() override {
    DVLOG(1) << "DestructFlow::" << __func__;
    weak_factory_.InvalidateWeakPtrs();
    releaser_->Cancel();
  }

  void CodecReleased() {
    DVLOG(1) << "DestructFlow::" << __func__;
    // This will delete |this| too.
    delete mcvd_;
  }

  std::unique_ptr<Flow> releaser_;
  MediaCodecVideoDecoder* mcvd_;
  base::WeakPtrFactory<DestructFlow> weak_factory_;
};

// ResetFlow
//  * Release owned output buffers back to the codec; no need to render them.
//  * Abort pending decode requests.
//  * Flush the decoder or drain if necessary.
//  * We don't need a needs_reset flag like AVDA to defer reset because the
//    VideoDecoder interface gives us better guarantees.
//     - End of video: EOS will be queued and decoded, no call to Reset().
//     - MSE config change: EOS queued, Reset() called after EOS is returned.
//       (We can lose a few frames on this transition but that's not new).
//     - Seek: Reset(). Don't care about old frames.
class ResetFlow : public Flow {
 public:
  ResetFlow(State* state, base::Closure done_cb)
      : Flow(state, std::move(done_cb)), weak_factory_(this) {}
  void Start() override {
    DVLOG(1) << "ResetFlow::" << __func__;
    // ReleaseOwnedBuffers();
    // AbortPendingDecodeRequests();

    if (!state_->codec || state_->codec->IsFlushed()) {
      PostDoneCb();
      return;
    }

    // If the codec can be flushed, just flush it and we're done.
    if (state_->codec->SupportsFlush()) {
      state_->codec->Flush();
      PostDoneCb();
      return;
    }

    // If the codec can't be flushed, release it.
    releaser_ = base::MakeUnique<ReleaseCodecFlow>(
        state_,
        base::Bind(&ResetFlow::CodecReleased, weak_factory_.GetWeakPtr()));
    releaser_->Start();
  }

  void Cancel() override {
    DVLOG(1) << "ResetFlow::" << __func__;
    weak_factory_.InvalidateWeakPtrs();
    if (releaser_)
      releaser_->Cancel();
  }

  void CodecReleased() {
    DVLOG(1) << "ResetFlow::" << __func__;
    base::ResetAndReturn(&state_->reset_cb).Run();
    PostDoneCb();
  }

  std::unique_ptr<ReleaseCodecFlow> releaser_;
  base::WeakPtrFactory<ResetFlow> weak_factory_;
};

// DecodeFlow
//  * Queues input and dequeues output.
//
// XXX: This never ends right now. It should probably end when it
// dequeues EOS so we can use it for draining. It might have to be configurable.
class DecodeFlow : public Flow {
 public:
  enum class CodecStatus { kOk, kTryAgain, kError };

  DecodeFlow(State* state, base::Closure done_cb) : Flow(state, done_cb) {}

  void Start() override {
    DVLOG(1) << "DecodeFlow::" << __func__;
    DCHECK(state_->codec);

    if (state_->has_pending_decodes_) {
      repeating_timer_.Start(
          base::Bind(&DecodeFlow::PumpDecoder, base::Unretained(this)));
    }
  }

  void Cancel() override {
    DVLOG(1) << "DecodeFlow::" << __func__;
    repeating_timer_.Stop();
  }

  // XXX Awkward that this can arrive after Cancel(). Maybe Flow should track
  // canceled state and ignore events when it's canceled? Or MCVD shouldn't
  // deliver these if the flow is canceled.
  void DecodeEvent() override {
    repeating_timer_.Start(
        base::Bind(&DecodeFlow::PumpDecoder, base::Unretained(this)));
  }

  void PumpDecoder() {
    DVLOG(3) << "DecodeFlow::" << __func__;
    // XXX: Consider guaranteeing that this won't run after an error.
    if (status_ == Status::kError)
      return;

    // XXX: AVDA also does: picture_buffer_manager_.MaybeRenderEarly(), but I
    // think we can avoid this and contain the MaybeRenderEarly logic in
    // VideoFrameFactory.
    bool did_work = false;
    while (true) {
      CodecStatus input_status = QueueInput();
      if (input_status == CodecStatus::kError) {
        set_status(Status::kError);
        PostDoneCb();
        return;
      }

      CodecStatus output_status = DequeueOutput();
      if (output_status == CodecStatus::kError) {
        set_status(Status::kError);
        PostDoneCb();
        return;
      }

      if (input_status == CodecStatus::kOk ||
          output_status == CodecStatus::kOk) {
        did_work = true;
      } else {
        break;
      }
    };

    if (!idle_timer_ || did_work)
      idle_timer_ = base::ElapsedTimer();
    if (idle_timer_->Elapsed() > kCodecIdleTimeout)
      repeating_timer_.Stop();
  }

  CodecStatus QueueInput() {
    DVLOG(3) << "DecodeFlow::" << __func__;

    if (state_->pending_decodes.empty())
      return CodecStatus::kTryAgain;

    int codec_buffer = -1;
    MediaCodecStatus status =
        state_->codec->DequeueInputBuffer(base::TimeDelta(), &codec_buffer);
    switch (status) {
      case MEDIA_CODEC_TRY_AGAIN_LATER:
        return CodecStatus::kTryAgain;
      case MEDIA_CODEC_ERROR:
        DVLOG(1) << "DequeueInputBuffer() failed";
        return CodecStatus::kError;
      case MEDIA_CODEC_OK:
        break;
      default:
        NOTREACHED();
        return CodecStatus::kError;
    }

    DCHECK_GE(codec_buffer, 0);
    PendingDecode input = state_->pending_decodes.front();
    state_->pending_decodes.pop_front();

    if (input.IsEOS()) {
      state_->codec->QueueEOS(codec_buffer);
      input.decode_cb.Run(DecodeStatus::OK);
      return CodecStatus::kOk;
    }

    MediaCodecStatus queue_status = state_->codec->QueueInputBuffer(
        codec_buffer, input.buffer->data(), input.buffer->data_size(),
        input.buffer->timestamp());
    DVLOG(2) << ": QueueInputBuffer(pts="
             << input.buffer->timestamp().InMilliseconds()
             << ") status=" << queue_status;

    if (queue_status == MEDIA_CODEC_OK) {
      input.decode_cb.Run(DecodeStatus::OK);
      return CodecStatus::kOk;
    } else {
      input.decode_cb.Run(DecodeStatus::DECODE_ERROR);
      return CodecStatus::kError;
    }
  }

  CodecStatus DequeueOutput() {
    DVLOG(3) << "DecodeFlow::" << __func__;

    int32_t codec_buffer = 0;
    size_t offset = 0, size = 0;
    base::TimeDelta presentation_time;
    bool eos = false;
    MediaCodecStatus status = state_->codec->DequeueOutputBuffer(
        base::TimeDelta(), &codec_buffer, &offset, &size, &presentation_time,
        &eos, NULL);

    switch (status) {
      case MEDIA_CODEC_ERROR:
        DVLOG(1) << "DequeueOutputBuffer() failed";
        return CodecStatus::kError;
      case MEDIA_CODEC_TRY_AGAIN_LATER:
      case MEDIA_CODEC_OUTPUT_FORMAT_CHANGED:
      case MEDIA_CODEC_OUTPUT_BUFFERS_CHANGED:
        return CodecStatus::kTryAgain;
      case MEDIA_CODEC_OK:
        break;
      default:
        NOTREACHED();
        return CodecStatus::kError;
    }

    DCHECK_GE(codec_buffer, 0);
    DVLOG(2) << "DequeueOutputBuffer(): pts=" << presentation_time
             << " codec_buffer=" << codec_buffer << " offset=" << offset
             << " size=" << size << " eos=" << eos;

    // XXX: Toggle pending EOS.
    if (eos)
      return CodecStatus::kOk;

    // XXX: What happens if output_cb is run after this is Reset()? That
    // probably violates the VideoDecoder contract.
    state_->video_frame_factory->NewVideoFrame(state_->codec.get(),
                                               codec_buffer, presentation_time,
                                               state_->output_cb);
    return CodecStatus::kOk;
  }

 private:
  base::Optional<base::ElapsedTimer> idle_timer_;
  CoalescingTimer repeating_timer_;
};

bool ShouldDeferFirstInit(AVDACodecAllocator* codec_allocator,
                          int surface_id,
                          VideoCodec codec) {
  return surface_id == SurfaceManager::kNoSurfaceID && codec == kCodecH264 &&
         codec_allocator->IsAnyRegisteredAVDA() &&
         (base::android::BuildInfo::GetInstance()->sdk_int() <= 18 ||
          base::SysInfo::IsLowEndDevice());
}

MediaCodecVideoDecoder::MediaCodecVideoDecoder(
    scoped_refptr<base::SingleThreadTaskRunner> gpu_task_runner,
    base::Callback<gpu::GpuCommandBufferStub*()> get_command_buffer_stub_cb,
    AVDACodecAllocator* codec_allocator)
    : reset_pending_(false),
      destruction_pending_(false),
      gpu_task_runner_(gpu_task_runner),
      get_command_buffer_stub_cb_(get_command_buffer_stub_cb),
      codec_allocator_(codec_allocator),
      // XXX: inject it.
      surface_chooser_(new AndroidVideoSurfaceChooserImpl()),
      flow_weak_factory_(this),
      weak_factory_(this) {}

MediaCodecVideoDecoder::~MediaCodecVideoDecoder() {}

void MediaCodecVideoDecoder::Initialize(const VideoDecoderConfig& config,
                                        bool low_delay,
                                        CdmContext* cdm_context,
                                        const InitCB& init_cb,
                                        const OutputCB& output_cb) {
  DVLOG(1) << __func__;
  InitCB bound_init_cb = BindToCurrentLoop(init_cb);

  if (!ConfigSupported(config)) {
    bound_init_cb.Run(false);
    return;
  }

  if (!codec_allocator_->StartThread(&codec_allocator_client_)) {
    bound_init_cb.Run(false);
    return;
  }

  bool is_first_init = !did_first_init_;
  did_first_init_ = true;

  if (is_first_init) {
    surface_chooser_->Initialize(
        base::Bind(&MediaCodecVideoDecoder::OnSurfaceTransition,
                   weak_factory_.GetWeakPtr()),
        base::Bind(&MediaCodecVideoDecoder::OnSurfaceTransition,
                   weak_factory_.GetWeakPtr(), nullptr),
        base::Bind(&MediaCodecVideoDecoder::SurfaceDestroyed,
                   weak_factory_.GetWeakPtr()),
        AndroidOverlayFactoryCB());
  }

  state_.config = config;
  state_.output_cb = BindToCurrentLoop(output_cb);
  bound_init_cb.Run(true);
  StartNextFlow();
}

void MediaCodecVideoDecoder::HandleFlowFailure() {
  DVLOG(1) << __func__;
  status_ = Status::kError;
  // For each pending_decode: run decode_cb with status failure.
}

void MediaCodecVideoDecoder::StartNextFlow() {
  DVLOG(1) << __func__;
  flow_weak_factory_.InvalidateWeakPtrs();

  if (flow_) {
    flow_->Cancel();
    if (flow_->failed()) {
      HandleFlowFailure();
      flow_.reset();
      return;
    }
  }

  base::Closure done_cb = base::Bind(&MediaCodecVideoDecoder::StartNextFlow,
                                     flow_weak_factory_.GetWeakPtr());

  if (destruction_pending_) {
    flow_ = base::MakeUnique<DestructFlow>(&state_, done_cb, this);
  } else if (status_ == Status::kError) {
    // If we're in an error state only DestructFlow should run.
    return;
  } else if (reset_pending_) {
    flow_ = base::MakeUnique<ResetFlow>(&state_, done_cb);
  } else if (!state_.codec && !state_.has_pending_decodes_) {
    // Wait for a pending decode before initializing.
    return;
  } else if (!state_.video_frame_factory) {
    flow_ = base::MakeUnique<InitVideoFrameFactoryFlow>(
        &state_, done_cb, gpu_task_runner_, get_command_buffer_stub_cb_);
  } else if (!state_.codec) {
    flow_ =
        base::MakeUnique<CreateCodecFlow>(&state_, done_cb, codec_allocator_);
    // } else if (NeedsSetSurface()) {
    // if (CanSwitchSurfaceWithoutReleasingCodec())
    //   setSurface();
    // else
    //   flow_ = base::MakeUnique<ReleaseCodecFlow>();
  } else {
    flow_ = base::MakeUnique<DecodeFlow>(&state_, done_cb);
  }

  flow_->Start();
}

void MediaCodecVideoDecoder::Destroy() {
  DVLOG(1) << __func__;
  destruction_pending_ = true;
  StartNextFlow();
}

void MediaCodecVideoDecoder::OnSurfaceTransition(
    std::unique_ptr<AndroidOverlay> overlay) {
  DVLOG(1) << __func__;
  state_.overlay = std::move(overlay);

  if (!flow_)
    return;

  StartNextFlow();
}

// This is the one thing that happens outside of a flow.
void MediaCodecVideoDecoder::SurfaceDestroyed(AndroidOverlay* overlay) {
  DVLOG(1) << __func__;

  // UpdateSurfaceSync();
  StartNextFlow();
}

void MediaCodecVideoDecoder::Decode(const scoped_refptr<DecoderBuffer>& buffer,
                                    const DecodeCB& decode_cb) {
  DVLOG(1) << __func__;
  // After an error all decodes fail.
  if (status_ == Status::kError)
    decode_cb.Run(DecodeStatus::DECODE_ERROR);

  state_.pending_decodes.push_back(PendingDecode{buffer, std::move(decode_cb)});
  if (flow_)
    flow_->DecodeEvent();
  else
    StartNextFlow();
}

void MediaCodecVideoDecoder::Reset(const base::Closure& reset_cb) {
  DVLOG(1) << __func__;
  if (status_ == Status::kError) {
    reset_cb.Run();
    return;
  }

  reset_pending_ = true;
  state_.reset_cb = reset_cb;
  StartNextFlow();
}

std::string MediaCodecVideoDecoder::GetDisplayName() const {
  return "MediaCodecVideoDecoder";
}

bool MediaCodecVideoDecoder::NeedsBitstreamConversion() const {
  return true;
}

bool MediaCodecVideoDecoder::CanReadWithoutStalling() const {
  return false;
}

int MediaCodecVideoDecoder::GetMaxDecodeRequests() const {
  return 4;
}

}  // namespace media
