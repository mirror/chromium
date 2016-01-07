// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/gpu/media/android_video_decode_accelerator.h"

#include <stddef.h>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "base/metrics/histogram.h"
#include "base/trace_event/trace_event.h"
#include "content/common/gpu/gpu_channel.h"
#include "content/common/gpu/media/android_copying_backing_strategy.h"
#include "content/common/gpu/media/android_deferred_rendering_backing_strategy.h"
#include "content/common/gpu/media/avda_return_on_failure.h"
#include "gpu/command_buffer/service/gles2_cmd_decoder.h"
#include "media/base/bitstream_buffer.h"
#include "media/base/limits.h"
#include "media/base/media_switches.h"
#include "media/base/timestamp_constants.h"
#include "media/base/video_decoder_config.h"
#include "media/video/picture.h"
#include "ui/gl/android/scoped_java_surface.h"
#include "ui/gl/android/surface_texture.h"
#include "ui/gl/gl_bindings.h"

#if defined(ENABLE_MOJO_MEDIA_IN_GPU_PROCESS)
#include "media/base/media_keys.h"
#include "media/mojo/services/mojo_cdm_service.h"
#endif

namespace content {

// TODO(liberato): It is unclear if we have an issue with deadlock during
// playback if we lower this.  Previously (crbug.com/176036), a deadlock
// could occur during preroll.  More recent tests have shown some
// instability with kNumPictureBuffers==2 with similar symptoms
// during playback.  crbug.com/531588 .
enum { kNumPictureBuffers = media::limits::kMaxVideoFrames + 1 };

// Max number of bitstreams notified to the client with
// NotifyEndOfBitstreamBuffer() before getting output from the bitstream.
enum { kMaxBitstreamsNotifiedInAdvance = 32 };

// MediaCodec is only guaranteed to support baseline, but some devices may
// support others. Advertise support for all H264 profiles and let the
// MediaCodec fail when decoding if it's not actually supported. It's assumed
// that consumers won't have software fallback for H264 on Android anyway.
static const media::VideoCodecProfile kSupportedH264Profiles[] = {
  media::H264PROFILE_BASELINE,
  media::H264PROFILE_MAIN,
  media::H264PROFILE_EXTENDED,
  media::H264PROFILE_HIGH,
  media::H264PROFILE_HIGH10PROFILE,
  media::H264PROFILE_HIGH422PROFILE,
  media::H264PROFILE_HIGH444PREDICTIVEPROFILE,
  media::H264PROFILE_SCALABLEBASELINE,
  media::H264PROFILE_SCALABLEHIGH,
  media::H264PROFILE_STEREOHIGH,
  media::H264PROFILE_MULTIVIEWHIGH
};

// Because MediaCodec is thread-hostile (must be poked on a single thread) and
// has no callback mechanism (b/11990118), we must drive it by polling for
// complete frames (and available input buffers, when the codec is fully
// saturated).  This function defines the polling delay.  The value used is an
// arbitrary choice that trades off CPU utilization (spinning) against latency.
// Mirrors android_video_encode_accelerator.cc:EncodePollDelay().
static inline const base::TimeDelta DecodePollDelay() {
  // An alternative to this polling scheme could be to dedicate a new thread
  // (instead of using the ChildThread) to run the MediaCodec, and make that
  // thread use the timeout-based flavor of MediaCodec's dequeue methods when it
  // believes the codec should complete "soon" (e.g. waiting for an input
  // buffer, or waiting for a picture when it knows enough complete input
  // pictures have been fed to saturate any internal buffering).  This is
  // speculative and it's unclear that this would be a win (nor that there's a
  // reasonably device-agnostic way to fill in the "believes" above).
  return base::TimeDelta::FromMilliseconds(10);
}

static inline const base::TimeDelta NoWaitTimeOut() {
  return base::TimeDelta::FromMicroseconds(0);
}

static inline const base::TimeDelta IdleTimerTimeOut() {
  return base::TimeDelta::FromSeconds(1);
}

// Handle OnFrameAvailable callbacks safely.  Since they occur asynchronously,
// we take care that the AVDA that wants them still exists.  A WeakPtr to
// the AVDA would be preferable, except that OnFrameAvailable callbacks can
// occur off the gpu main thread.  We also can't guarantee when the
// SurfaceTexture will quit sending callbacks to coordinate with the
// destruction of the AVDA, so we have a separate object that the cb can own.
class AndroidVideoDecodeAccelerator::OnFrameAvailableHandler
    : public base::RefCountedThreadSafe<OnFrameAvailableHandler> {
 public:
  // We do not retain ownership of |owner|.  It must remain valid until
  // after ClearOwner() is called.  This will register with
  // |surface_texture| to receive OnFrameAvailable callbacks.
  OnFrameAvailableHandler(
      AndroidVideoDecodeAccelerator* owner,
      const scoped_refptr<gfx::SurfaceTexture>& surface_texture)
      : owner_(owner) {
    // Note that the callback owns a strong ref to us.
    surface_texture->SetFrameAvailableCallbackOnAnyThread(
        base::Bind(&OnFrameAvailableHandler::OnFrameAvailable,
                   scoped_refptr<OnFrameAvailableHandler>(this)));
  }

  // Forget about our owner, which is required before one deletes it.
  // No further callbacks will happen once this completes.
  void ClearOwner() {
    base::AutoLock lock(lock_);
    // No callback can happen until we release the lock.
    owner_ = nullptr;
  }

  // Call back into our owner if it hasn't been deleted.
  void OnFrameAvailable() {
    base::AutoLock auto_lock(lock_);
    // |owner_| can't be deleted while we have the lock.
    if (owner_)
      owner_->OnFrameAvailable();
  }

 private:
  friend class base::RefCountedThreadSafe<OnFrameAvailableHandler>;
  virtual ~OnFrameAvailableHandler() {}

  // Protects changes to owner_.
  base::Lock lock_;

  // AVDA that wants the OnFrameAvailable callback.
  AndroidVideoDecodeAccelerator* owner_;

  DISALLOW_COPY_AND_ASSIGN(OnFrameAvailableHandler);
};

AndroidVideoDecodeAccelerator::AndroidVideoDecodeAccelerator(
    const base::WeakPtr<gpu::gles2::GLES2Decoder> decoder,
    const base::Callback<bool(void)>& make_context_current)
    : client_(NULL),
      make_context_current_(make_context_current),
      codec_(media::kCodecH264),
      is_encrypted_(false),
      state_(NO_ERROR),
      picturebuffers_requested_(false),
      gl_decoder_(decoder),
      weak_this_factory_(this) {
  if (UseDeferredRenderingStrategy())
    strategy_.reset(new AndroidDeferredRenderingBackingStrategy());
  else
    strategy_.reset(new AndroidCopyingBackingStrategy());
}

AndroidVideoDecodeAccelerator::~AndroidVideoDecodeAccelerator() {
  DCHECK(thread_checker_.CalledOnValidThread());
}

bool AndroidVideoDecodeAccelerator::Initialize(const Config& config,
                                               Client* client) {
  DCHECK(!media_codec_);
  DCHECK(thread_checker_.CalledOnValidThread());
  TRACE_EVENT0("media", "AVDA::Initialize");

  DVLOG(1) << __FUNCTION__ << ": profile:" << config.profile
           << " is_encrypted:" << config.is_encrypted;

  client_ = client;
  codec_ = VideoCodecProfileToVideoCodec(config.profile);
  is_encrypted_ = config.is_encrypted;

  bool profile_supported = codec_ == media::kCodecVP8 ||
                           codec_ == media::kCodecVP9 ||
                           codec_ == media::kCodecH264;

  if (!profile_supported) {
    LOG(ERROR) << "Unsupported profile: " << config.profile;
    return false;
  }

  // Only use MediaCodec for VP8/9 if it's likely backed by hardware.
  if ((codec_ == media::kCodecVP8 || codec_ == media::kCodecVP9) &&
      media::VideoCodecBridge::IsKnownUnaccelerated(
          codec_, media::MEDIA_CODEC_DECODER)) {
    DVLOG(1) << "Initialization failed: "
             << (codec_ == media::kCodecVP8 ? "vp8" : "vp9")
             << " is not hardware accelerated";
    return false;
  }

  if (!make_context_current_.Run()) {
    LOG(ERROR) << "Failed to make this decoder's GL context current.";
    return false;
  }

  if (!gl_decoder_) {
    LOG(ERROR) << "Failed to get gles2 decoder instance.";
    return false;
  }

  strategy_->Initialize(this);

  surface_texture_ = strategy_->CreateSurfaceTexture();
  on_frame_available_handler_ =
      new OnFrameAvailableHandler(this, surface_texture_);

  if (!ConfigureMediaCodec()) {
    LOG(ERROR) << "Failed to create MediaCodec instance.";
    return false;
  }

  return true;
}

void AndroidVideoDecodeAccelerator::SetCdm(int cdm_id) {
  DVLOG(2) << __FUNCTION__ << ": " << cdm_id;

#if defined(ENABLE_MOJO_MEDIA_IN_GPU_PROCESS)
  // TODO(timav): Implement CDM setting here. See http://crbug.com/542417
  scoped_refptr<media::MediaKeys> cdm = media::MojoCdmService::GetCdm(cdm_id);
  DCHECK(cdm);
#endif

  NOTIMPLEMENTED();
  NotifyCdmAttached(false);
}

void AndroidVideoDecodeAccelerator::DoIOTask() {
  DCHECK(thread_checker_.CalledOnValidThread());
  TRACE_EVENT0("media", "AVDA::DoIOTask");
  if (state_ == ERROR) {
    return;
  }

  bool did_work = QueueInput();
  while (DequeueOutput())
    did_work = true;

  ManageTimer(did_work);
}

bool AndroidVideoDecodeAccelerator::QueueInput() {
  DCHECK(thread_checker_.CalledOnValidThread());
  TRACE_EVENT0("media", "AVDA::QueueInput");
  if (bitstreams_notified_in_advance_.size() > kMaxBitstreamsNotifiedInAdvance)
    return false;
  if (pending_bitstream_buffers_.empty())
    return false;

  int input_buf_index = 0;
  media::MediaCodecStatus status =
      media_codec_->DequeueInputBuffer(NoWaitTimeOut(), &input_buf_index);
  if (status != media::MEDIA_CODEC_OK) {
    DCHECK(status == media::MEDIA_CODEC_DEQUEUE_INPUT_AGAIN_LATER ||
           status == media::MEDIA_CODEC_ERROR);
    return false;
  }

  base::Time queued_time = pending_bitstream_buffers_.front().second;
  UMA_HISTOGRAM_TIMES("Media.AVDA.InputQueueTime",
                      base::Time::Now() - queued_time);
  media::BitstreamBuffer bitstream_buffer =
      pending_bitstream_buffers_.front().first;
  pending_bitstream_buffers_.pop();
  TRACE_COUNTER1("media", "AVDA::PendingBitstreamBufferCount",
                 pending_bitstream_buffers_.size());

  if (bitstream_buffer.id() == -1) {
    media_codec_->QueueEOS(input_buf_index);
    return true;
  }

  scoped_ptr<base::SharedMemory> shm(
      new base::SharedMemory(bitstream_buffer.handle(), true));
  RETURN_ON_FAILURE(this, shm->Map(bitstream_buffer.size()),
                    "Failed to SharedMemory::Map()", UNREADABLE_INPUT, false);

  const base::TimeDelta presentation_timestamp =
      bitstream_buffer.presentation_timestamp();
  DCHECK(presentation_timestamp != media::kNoTimestamp())
      << "Bitstream buffers must have valid presentation timestamps";

  // There may already be a bitstream buffer with this timestamp, e.g., VP9 alt
  // ref frames, but it's OK to overwrite it because we only expect a single
  // output frame to have that timestamp. AVDA clients only use the bitstream
  // buffer id in the returned Pictures to map a bitstream buffer back to a
  // timestamp on their side, so either one of the bitstream buffer ids will
  // result in them finding the right timestamp.
  bitstream_buffers_in_decoder_[presentation_timestamp] = bitstream_buffer.id();

  const uint8_t* memory = static_cast<const uint8_t*>(shm->memory());
  const std::string& key_id = bitstream_buffer.key_id();
  const std::string& iv = bitstream_buffer.iv();
  const std::vector<media::SubsampleEntry>& subsamples =
      bitstream_buffer.subsamples();

  if (key_id.empty() || iv.empty()) {
    status = media_codec_->QueueInputBuffer(input_buf_index, memory,
                                            bitstream_buffer.size(),
                                            presentation_timestamp);
  } else {
    status = media_codec_->QueueSecureInputBuffer(
        input_buf_index, memory, bitstream_buffer.size(), key_id, iv,
        subsamples, presentation_timestamp);
  }

  DVLOG(2) << __FUNCTION__
           << ": QueueInputBuffer: pts:" << presentation_timestamp
           << " status:" << status;

  RETURN_ON_FAILURE(this, status == media::MEDIA_CODEC_OK,
                    "Failed to QueueInputBuffer: " << status, PLATFORM_FAILURE,
                    false);

  // We should call NotifyEndOfBitstreamBuffer(), when no more decoded output
  // will be returned from the bitstream buffer. However, MediaCodec API is
  // not enough to guarantee it.
  // So, here, we calls NotifyEndOfBitstreamBuffer() in advance in order to
  // keep getting more bitstreams from the client, and throttle them by using
  // |bitstreams_notified_in_advance_|.
  // TODO(dwkang): check if there is a way to remove this workaround.
  base::MessageLoop::current()->PostTask(
      FROM_HERE,
      base::Bind(&AndroidVideoDecodeAccelerator::NotifyEndOfBitstreamBuffer,
                 weak_this_factory_.GetWeakPtr(),
                 bitstream_buffer.id()));
  bitstreams_notified_in_advance_.push_back(bitstream_buffer.id());

  return true;
}

bool AndroidVideoDecodeAccelerator::DequeueOutput() {
  DCHECK(thread_checker_.CalledOnValidThread());
  TRACE_EVENT0("media", "AVDA::DequeueOutput");
  if (picturebuffers_requested_ && output_picture_buffers_.empty())
    return false;

  if (!output_picture_buffers_.empty() && free_picture_ids_.empty()) {
    // Don't have any picture buffer to send. Need to wait more.
    return false;
  }

  bool eos = false;
  base::TimeDelta presentation_timestamp;
  int32_t buf_index = 0;
  bool should_try_again = false;
  do {
    size_t offset = 0;
    size_t size = 0;

    TRACE_EVENT_BEGIN0("media", "AVDA::DequeueOutputBuffer");
    media::MediaCodecStatus status = media_codec_->DequeueOutputBuffer(
        NoWaitTimeOut(), &buf_index, &offset, &size, &presentation_timestamp,
        &eos, NULL);
    TRACE_EVENT_END2("media", "AVDA::DequeueOutputBuffer", "status", status,
                     "presentation_timestamp (ms)",
                     presentation_timestamp.InMilliseconds());

    DVLOG(3) << "AVDA::DequeueOutputBuffer: pts:" << presentation_timestamp
             << " buf_index:" << buf_index << " offset:" << offset
             << " size:" << size << " eos:" << eos;

    switch (status) {
      case media::MEDIA_CODEC_DEQUEUE_OUTPUT_AGAIN_LATER:
      case media::MEDIA_CODEC_ERROR:
        return false;

      case media::MEDIA_CODEC_OUTPUT_FORMAT_CHANGED: {
        int32_t width, height;
        media_codec_->GetOutputFormat(&width, &height);

        if (!picturebuffers_requested_) {
          picturebuffers_requested_ = true;
          size_ = gfx::Size(width, height);
          base::MessageLoop::current()->PostTask(
              FROM_HERE,
              base::Bind(&AndroidVideoDecodeAccelerator::RequestPictureBuffers,
                         weak_this_factory_.GetWeakPtr()));
        } else {
          // Dynamic resolution change support is not specified by the Android
          // platform at and before JB-MR1, so it's not possible to smoothly
          // continue playback at this point.  Instead, error out immediately,
          // expecting clients to Reset() as appropriate to avoid this.
          // b/7093648
          RETURN_ON_FAILURE(this, size_ == gfx::Size(width, height),
                            "Dynamic resolution change is not supported.",
                            PLATFORM_FAILURE, false);
        }
        return false;
      }

      case media::MEDIA_CODEC_OUTPUT_BUFFERS_CHANGED:
        break;

      case media::MEDIA_CODEC_OK:
        DCHECK_GE(buf_index, 0);
        break;

      default:
        NOTREACHED();
        break;
    }
  } while (buf_index < 0);

  // Normally we assume that the decoder makes at most one output frame for each
  // distinct input timestamp. A VP9 alt ref frame is a case where an input
  // buffer, with a possibly unique timestamp, will not result in a
  // corresponding output frame.

  // However MediaCodecBridge uses timestamp correction and provides
  // non-decreasing timestamp sequence, which might result in timestamp
  // duplicates. Discard the frame if we cannot get corresponding buffer id.

  // Get the bitstream buffer id from the timestamp.
  auto it = eos ? bitstream_buffers_in_decoder_.end()
                : bitstream_buffers_in_decoder_.find(presentation_timestamp);

  if (it == bitstream_buffers_in_decoder_.end()) {
    media_codec_->ReleaseOutputBuffer(buf_index, false);
    base::MessageLoop::current()->PostTask(
        FROM_HERE,
        base::Bind(&AndroidVideoDecodeAccelerator::NotifyFlushDone,
                   weak_this_factory_.GetWeakPtr()));
  } else {
    const int32_t bitstream_buffer_id = it->second;
    bitstream_buffers_in_decoder_.erase(bitstream_buffers_in_decoder_.begin(),
                                        ++it);
    SendCurrentSurfaceToClient(buf_index, bitstream_buffer_id);

    // If we decoded a frame this time, then try for another.
    should_try_again = true;

    // Removes ids former or equal than the id from decoder. Note that
    // |bitstreams_notified_in_advance_| does not mean bitstream ids in decoder
    // because of frame reordering issue. We just maintain this roughly and use
    // for the throttling purpose.
    for (auto bitstream_it = bitstreams_notified_in_advance_.begin();
         bitstream_it != bitstreams_notified_in_advance_.end();
         ++bitstream_it) {
      if (*bitstream_it == bitstream_buffer_id) {
        bitstreams_notified_in_advance_.erase(
            bitstreams_notified_in_advance_.begin(), ++bitstream_it);
        break;
      }
    }
  }

  return should_try_again;
}

void AndroidVideoDecodeAccelerator::SendCurrentSurfaceToClient(
    int32_t codec_buffer_index,
    int32_t bitstream_id) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK_NE(bitstream_id, -1);
  DCHECK(!free_picture_ids_.empty());
  TRACE_EVENT0("media", "AVDA::SendCurrentSurfaceToClient");

  RETURN_ON_FAILURE(this, make_context_current_.Run(),
                    "Failed to make this decoder's GL context current.",
                    PLATFORM_FAILURE);

  int32_t picture_buffer_id = free_picture_ids_.front();
  free_picture_ids_.pop();
  TRACE_COUNTER1("media", "AVDA::FreePictureIds", free_picture_ids_.size());

  OutputBufferMap::const_iterator i =
      output_picture_buffers_.find(picture_buffer_id);
  RETURN_ON_FAILURE(this, i != output_picture_buffers_.end(),
                    "Can't find a PictureBuffer for " << picture_buffer_id,
                    PLATFORM_FAILURE);

  // Connect the PictureBuffer to the decoded frame, via whatever
  // mechanism the strategy likes.
  strategy_->UseCodecBufferForPictureBuffer(codec_buffer_index, i->second);

  // TODO(henryhsu): Pass (0, 0) as visible size will cause several test
  // cases failed. We should make sure |size_| is coded size or visible size.
  base::MessageLoop::current()->PostTask(
      FROM_HERE, base::Bind(&AndroidVideoDecodeAccelerator::NotifyPictureReady,
                            weak_this_factory_.GetWeakPtr(),
                            media::Picture(picture_buffer_id, bitstream_id,
                                           gfx::Rect(size_), false)));
}

void AndroidVideoDecodeAccelerator::Decode(
    const media::BitstreamBuffer& bitstream_buffer) {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (bitstream_buffer.id() != -1 && bitstream_buffer.size() == 0) {
    base::MessageLoop::current()->PostTask(
        FROM_HERE,
        base::Bind(&AndroidVideoDecodeAccelerator::NotifyEndOfBitstreamBuffer,
                   weak_this_factory_.GetWeakPtr(), bitstream_buffer.id()));
    return;
  }

  pending_bitstream_buffers_.push(
      std::make_pair(bitstream_buffer, base::Time::Now()));
  TRACE_COUNTER1("media", "AVDA::PendingBitstreamBufferCount",
                 pending_bitstream_buffers_.size());

  DoIOTask();
}

void AndroidVideoDecodeAccelerator::RequestPictureBuffers() {
  client_->ProvidePictureBuffers(kNumPictureBuffers, size_,
                                 strategy_->GetTextureTarget());
}

void AndroidVideoDecodeAccelerator::AssignPictureBuffers(
    const std::vector<media::PictureBuffer>& buffers) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(output_picture_buffers_.empty());
  DCHECK(free_picture_ids_.empty());

  for (size_t i = 0; i < buffers.size(); ++i) {
    RETURN_ON_FAILURE(this, buffers[i].size() == size_,
                      "Invalid picture buffer size was passed.",
                      INVALID_ARGUMENT);
    int32_t id = buffers[i].id();
    output_picture_buffers_.insert(std::make_pair(id, buffers[i]));
    free_picture_ids_.push(id);
    // Since the client might be re-using |picture_buffer_id| values, forget
    // about previously-dismissed IDs now.  See ReusePictureBuffer() comment
    // about "zombies" for why we maintain this set in the first place.
    dismissed_picture_ids_.erase(id);

    strategy_->AssignOnePictureBuffer(buffers[i]);
  }
  TRACE_COUNTER1("media", "AVDA::FreePictureIds", free_picture_ids_.size());

  RETURN_ON_FAILURE(this, output_picture_buffers_.size() >= kNumPictureBuffers,
                    "Invalid picture buffers were passed.", INVALID_ARGUMENT);

  DoIOTask();
}

void AndroidVideoDecodeAccelerator::ReusePictureBuffer(
    int32_t picture_buffer_id) {
  DCHECK(thread_checker_.CalledOnValidThread());

  // This ReusePictureBuffer() might have been in a pipe somewhere (queued in
  // IPC, or in a PostTask either at the sender or receiver) when we sent a
  // DismissPictureBuffer() for this |picture_buffer_id|.  Account for such
  // potential "zombie" IDs here.
  if (dismissed_picture_ids_.erase(picture_buffer_id))
    return;

  free_picture_ids_.push(picture_buffer_id);

  OutputBufferMap::const_iterator i =
      output_picture_buffers_.find(picture_buffer_id);
  RETURN_ON_FAILURE(this, i != output_picture_buffers_.end(),
                    "Can't find a PictureBuffer for " << picture_buffer_id,
                    PLATFORM_FAILURE);
  strategy_->ReuseOnePictureBuffer(i->second);

  TRACE_COUNTER1("media", "AVDA::FreePictureIds", free_picture_ids_.size());

  DoIOTask();
}

void AndroidVideoDecodeAccelerator::Flush() {
  DCHECK(thread_checker_.CalledOnValidThread());

  Decode(media::BitstreamBuffer(-1, base::SharedMemoryHandle(), 0));
}

bool AndroidVideoDecodeAccelerator::ConfigureMediaCodec() {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(surface_texture_.get());
  TRACE_EVENT0("media", "AVDA::ConfigureMediaCodec");

  gfx::ScopedJavaSurface surface(surface_texture_.get());

  // Pass a dummy 320x240 canvas size and let the codec signal the real size
  // when it's known from the bitstream.
  media_codec_.reset(media::VideoCodecBridge::CreateDecoder(
      codec_, false, gfx::Size(320, 240), surface.j_surface().obj(), NULL));
  strategy_->CodecChanged(media_codec_.get(), output_picture_buffers_);
  if (!media_codec_)
    return false;

  ManageTimer(true);
  return true;
}

void AndroidVideoDecodeAccelerator::Reset() {
  DCHECK(thread_checker_.CalledOnValidThread());
  TRACE_EVENT0("media", "AVDA::Reset");

  while (!pending_bitstream_buffers_.empty()) {
    int32_t bitstream_buffer_id = pending_bitstream_buffers_.front().first.id();
    pending_bitstream_buffers_.pop();

    if (bitstream_buffer_id != -1) {
      base::MessageLoop::current()->PostTask(
          FROM_HERE,
          base::Bind(&AndroidVideoDecodeAccelerator::NotifyEndOfBitstreamBuffer,
                     weak_this_factory_.GetWeakPtr(), bitstream_buffer_id));
    }
  }
  TRACE_COUNTER1("media", "AVDA::PendingBitstreamBufferCount", 0);
  bitstreams_notified_in_advance_.clear();

  for (OutputBufferMap::iterator it = output_picture_buffers_.begin();
       it != output_picture_buffers_.end();
       ++it) {
    strategy_->DismissOnePictureBuffer(it->second);
    client_->DismissPictureBuffer(it->first);
    dismissed_picture_ids_.insert(it->first);
  }
  output_picture_buffers_.clear();
  std::queue<int32_t> empty;
  std::swap(free_picture_ids_, empty);
  CHECK(free_picture_ids_.empty());
  picturebuffers_requested_ = false;
  bitstream_buffers_in_decoder_.clear();

  // On some devices, and up to at least JB-MR1,
  // - flush() can fail after EOS (b/8125974); and
  // - mid-stream resolution change is unsupported (b/7093648).
  // To cope with these facts, we always stop & restart the codec on Reset().
  io_timer_.Stop();
  media_codec_->Stop();
  ConfigureMediaCodec();
  state_ = NO_ERROR;

  base::MessageLoop::current()->PostTask(
      FROM_HERE,
      base::Bind(&AndroidVideoDecodeAccelerator::NotifyResetDone,
                 weak_this_factory_.GetWeakPtr()));
}

void AndroidVideoDecodeAccelerator::Destroy() {
  DCHECK(thread_checker_.CalledOnValidThread());

  strategy_->Cleanup(output_picture_buffers_);

  // If we have an OnFrameAvailable handler, tell it that we're going away.
  if (on_frame_available_handler_) {
    on_frame_available_handler_->ClearOwner();
    on_frame_available_handler_ = nullptr;
  }

  weak_this_factory_.InvalidateWeakPtrs();
  if (media_codec_) {
    io_timer_.Stop();
    media_codec_->Stop();
  }
  delete this;
}

bool AndroidVideoDecodeAccelerator::CanDecodeOnIOThread() {
  return false;
}

const gfx::Size& AndroidVideoDecodeAccelerator::GetSize() const {
  return size_;
}

const base::ThreadChecker& AndroidVideoDecodeAccelerator::ThreadChecker()
    const {
  return thread_checker_;
}

base::WeakPtr<gpu::gles2::GLES2Decoder>
AndroidVideoDecodeAccelerator::GetGlDecoder() const {
  return gl_decoder_;
}

void AndroidVideoDecodeAccelerator::OnFrameAvailable() {
  // Remember: this may be on any thread.
  DCHECK(strategy_);
  strategy_->OnFrameAvailable();
}

void AndroidVideoDecodeAccelerator::PostError(
    const ::tracked_objects::Location& from_here,
    media::VideoDecodeAccelerator::Error error) {
  base::MessageLoop::current()->PostTask(
      from_here, base::Bind(&AndroidVideoDecodeAccelerator::NotifyError,
                            weak_this_factory_.GetWeakPtr(), error));
  state_ = ERROR;
}

void AndroidVideoDecodeAccelerator::NotifyCdmAttached(bool success) {
  client_->NotifyCdmAttached(success);
}

void AndroidVideoDecodeAccelerator::NotifyPictureReady(
    const media::Picture& picture) {
  client_->PictureReady(picture);
}

void AndroidVideoDecodeAccelerator::NotifyEndOfBitstreamBuffer(
    int input_buffer_id) {
  client_->NotifyEndOfBitstreamBuffer(input_buffer_id);
}

void AndroidVideoDecodeAccelerator::NotifyFlushDone() {
  client_->NotifyFlushDone();
}

void AndroidVideoDecodeAccelerator::NotifyResetDone() {
  client_->NotifyResetDone();
}

void AndroidVideoDecodeAccelerator::NotifyError(
    media::VideoDecodeAccelerator::Error error) {
  client_->NotifyError(error);
}

void AndroidVideoDecodeAccelerator::ManageTimer(bool did_work) {
  bool should_be_running = true;

  base::TimeTicks now = base::TimeTicks::Now();
  if (!did_work) {
    // Make sure that we have done work recently enough, else stop the timer.
    if (now - most_recent_work_ > IdleTimerTimeOut())
      should_be_running = false;
  } else {
    most_recent_work_ = now;
  }

  if (should_be_running && !io_timer_.IsRunning()) {
    io_timer_.Start(FROM_HERE, DecodePollDelay(), this,
                    &AndroidVideoDecodeAccelerator::DoIOTask);
  } else if (!should_be_running && io_timer_.IsRunning()) {
    io_timer_.Stop();
  }
}

// static
bool AndroidVideoDecodeAccelerator::UseDeferredRenderingStrategy() {
  return base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kEnableUnifiedMediaPipeline);
}

// static
media::VideoDecodeAccelerator::Capabilities
AndroidVideoDecodeAccelerator::GetCapabilities() {
  Capabilities capabilities;
  SupportedProfiles& profiles = capabilities.supported_profiles;

  if (!media::VideoCodecBridge::IsKnownUnaccelerated(
          media::kCodecVP8, media::MEDIA_CODEC_DECODER)) {
    SupportedProfile profile;
    profile.profile = media::VP8PROFILE_ANY;
    profile.min_resolution.SetSize(0, 0);
    profile.max_resolution.SetSize(1920, 1088);
    profiles.push_back(profile);
  }

  if (!media::VideoCodecBridge::IsKnownUnaccelerated(
          media::kCodecVP9, media::MEDIA_CODEC_DECODER)) {
    SupportedProfile profile;
    profile.profile = media::VP9PROFILE_ANY;
    profile.min_resolution.SetSize(0, 0);
    profile.max_resolution.SetSize(1920, 1088);
    profiles.push_back(profile);
  }

  for (const auto& supported_profile : kSupportedH264Profiles) {
    SupportedProfile profile;
    profile.profile = supported_profile;
    profile.min_resolution.SetSize(0, 0);
    // Advertise support for 4k and let the MediaCodec fail when decoding if it
    // doesn't support the resolution. It's assumed that consumers won't have
    // software fallback for H264 on Android anyway.
    profile.max_resolution.SetSize(3840, 2160);
    profiles.push_back(profile);
  }

  if (UseDeferredRenderingStrategy()) {
    capabilities.flags = media::VideoDecodeAccelerator::Capabilities::
        NEEDS_ALL_PICTURE_BUFFERS_TO_DECODE;
  }

  return capabilities;
}

}  // namespace content
