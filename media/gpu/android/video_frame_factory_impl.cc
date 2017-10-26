// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/gpu/android/video_frame_factory_impl.h"

#include "base/bind.h"
#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "base/single_thread_task_runner.h"
#include "base/stl_util.h"
#include "base/task_runner_util.h"
#include "gpu/command_buffer/common/constants.h"
#include "gpu/command_buffer/service/gles2_cmd_decoder.h"
#include "gpu/command_buffer/service/mailbox_manager.h"
#include "gpu/command_buffer/service/texture_manager.h"
#include "gpu/ipc/service/gpu_channel.h"
#include "gpu/ipc/service/gpu_command_buffer_stub.h"
#include "media/base/bind_to_current_loop.h"
#include "media/base/scoped_callback_runner.h"
#include "media/base/sequenced_weak_ptr.h"
#include "media/base/video_frame.h"
#include "media/gpu//android/codec_image.h"
#include "media/gpu/android/codec_wrapper.h"
#include "ui/gl/android/surface_texture.h"
#include "ui/gl/gl_bindings.h"

namespace media {
namespace {

bool MakeContextCurrent(gpu::GpuCommandBufferStub* stub) {
  return stub && stub->decoder()->MakeCurrent();
}

}  // namespace

VideoFrameFactoryImpl::VideoFrameFactoryImpl(GetStubCb get_stub_cb)
    : get_stub_cb_(std::move(get_stub_cb)) {}

VideoFrameFactoryImpl::~VideoFrameFactoryImpl() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (stub_)
    stub_->RemoveDestructionObserver(this);
  ClearTextureRefs();
}

void VideoFrameFactoryImpl::RunAfterPendingVideoFrames(
    base::OnceClosure closure) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // Since we've now hopped through our task runner, just run |closure|.
  std::move(closure).Run();
}

void VideoFrameFactoryImpl::Initialize(InitCb init_cb) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  stub_ = get_stub_cb_.Run();
  if (!MakeContextCurrent(stub_)) {
    init_cb.Run(nullptr);
    return;
  }
  stub_->AddDestructionObserver(this);
  decoder_helper_ = GLES2DecoderHelper::Create(stub_->decoder());
  init_cb.Run(SurfaceTextureGLOwnerImpl::Create());
}

void VideoFrameFactoryImpl::CreateVideoFrame(
    std::unique_ptr<CodecOutputBuffer> output_buffer,
    scoped_refptr<SurfaceTextureGLOwner> surface_texture,
    base::TimeDelta timestamp,
    gfx::Size natural_size,
    PromotionHintAggregator::NotifyPromotionHintCB promotion_hint_cb,
    VideoFrameFactory::OutputWithReleaseMailboxCB output_cb) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  scoped_refptr<VideoFrame> frame;
  scoped_refptr<gpu::gles2::TextureRef> texture_ref;
  CreateVideoFrameInternal(std::move(output_buffer), std::move(surface_texture),
                           timestamp, natural_size,
                           std::move(promotion_hint_cb), &frame, &texture_ref);
  if (!frame || !texture_ref)
    return;

  // Try to render this frame if possible.
  internal::MaybeRenderEarly(&images_);

  // TODO(sandersd, watk): The VideoFrame release callback will not be called
  // after MojoVideoDecoderService is destructed, so we have to release all
  // our TextureRefs when |this| is destructed. This can unback outstanding
  // VideoFrames (e.g., the current frame when the player is suspended). The
  // release callback lifetime should be separate from MCVD or
  // MojoVideoDecoderService (http://crbug.com/737220).
  texture_refs_[texture_ref.get()] = texture_ref;
  auto drop_texture_ref =
      this->BindRepeating(&VideoFrameFactoryImpl::DropTextureRef,
                          base::Unretained(texture_ref.get()));

  // Guarantee that the TextureRef is released even if the VideoFrame is
  // dropped. Otherwise we could keep TextureRefs we don't need alive.
  auto release_cb = ScopedCallbackRunner(
      ToOnceCallback(BindToCurrentLoop(drop_texture_ref)), gpu::SyncToken());
  output_cb.Run(std::move(release_cb), std::move(frame));
}

void VideoFrameFactoryImpl::CreateVideoFrameInternal(
    std::unique_ptr<CodecOutputBuffer> output_buffer,
    scoped_refptr<SurfaceTextureGLOwner> surface_texture,
    base::TimeDelta timestamp,
    gfx::Size natural_size,
    PromotionHintAggregator::NotifyPromotionHintCB promotion_hint_cb,
    scoped_refptr<VideoFrame>* video_frame_out,
    scoped_refptr<gpu::gles2::TextureRef>* texture_ref_out) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (!MakeContextCurrent(stub_))
    return;

  gpu::gles2::ContextGroup* group = stub_->decoder()->GetContextGroup();
  if (!group)
    return;
  gpu::gles2::TextureManager* texture_manager = group->texture_manager();
  if (!texture_manager)
    return;

  gfx::Size size = output_buffer->size();
  gfx::Rect visible_rect(size);
  // The pixel format doesn't matter as long as it's valid for texture frames.
  VideoPixelFormat pixel_format = PIXEL_FORMAT_ARGB;

  // Check that we can create a VideoFrame for this config before creating the
  // TextureRef so that we don't have to clean up the TextureRef if creating the
  // frame fails.
  if (!VideoFrame::IsValidConfig(pixel_format, VideoFrame::STORAGE_OPAQUE, size,
                                 visible_rect, natural_size)) {
    return;
  }

  // Create a Texture and a CodecImage to back it.
  scoped_refptr<gpu::gles2::TextureRef> texture_ref =
      decoder_helper_->CreateTexture(GL_TEXTURE_EXTERNAL_OES, GL_RGBA,
                                     size.width(), size.height(), GL_RGBA,
                                     GL_UNSIGNED_BYTE);
  // TODO(liberato): BindOnce
  auto image = base::MakeRefCounted<CodecImage>(
      std::move(output_buffer), surface_texture, std::move(promotion_hint_cb),
      this->BindRepeating(&VideoFrameFactoryImpl::OnImageDestructed));
  images_.push_back(image.get());

  // Attach the image to the texture.
  // If we're attaching a SurfaceTexture backed image, we set the state to
  // UNBOUND. This ensures that the implementation will call CopyTexImage()
  // which lets us update the surface texture at the right time.
  // For overlays we set the state to BOUND because it's required for
  // ScheduleOverlayPlane() to be called. If something tries to sample from an
  // overlay texture it won't work, but there's no way to make that work.
  auto image_state = surface_texture ? gpu::gles2::Texture::UNBOUND
                                     : gpu::gles2::Texture::BOUND;
  GLuint surface_texture_service_id =
      surface_texture ? surface_texture->GetTextureId() : 0;
  texture_manager->SetLevelStreamTextureImage(
      texture_ref.get(), GL_TEXTURE_EXTERNAL_OES, 0, image.get(), image_state,
      surface_texture_service_id);
  texture_manager->SetLevelCleared(texture_ref.get(), GL_TEXTURE_EXTERNAL_OES,
                                   0, true);

  gpu::Mailbox mailbox = decoder_helper_->CreateMailbox(texture_ref.get());
  gpu::MailboxHolder mailbox_holders[VideoFrame::kMaxPlanes];
  mailbox_holders[0] =
      gpu::MailboxHolder(mailbox, gpu::SyncToken(), GL_TEXTURE_EXTERNAL_OES);

  auto frame = VideoFrame::WrapNativeTextures(
      pixel_format, mailbox_holders, VideoFrame::ReleaseMailboxCB(), size,
      visible_rect, natural_size, timestamp);

  // The frames must be copied when threaded texture mailboxes are in use
  // (http://crbug.com/582170).
  if (stub_->GetGpuPreferences().enable_threaded_texture_mailboxes)
    frame->metadata()->SetBoolean(VideoFrameMetadata::COPY_REQUIRED, true);

  frame->metadata()->SetBoolean(VideoFrameMetadata::ALLOW_OVERLAY,
                                !surface_texture);

  *video_frame_out = std::move(frame);
  *texture_ref_out = std::move(texture_ref);
}

void VideoFrameFactoryImpl::OnWillDestroyStub() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(stub_);
  ClearTextureRefs();
  stub_ = nullptr;
  decoder_helper_ = nullptr;
}

void VideoFrameFactoryImpl::ClearTextureRefs() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(stub_ || texture_refs_.empty());
  // If we fail to make the context current, we have to notify the TextureRefs
  // so they don't try to delete textures without a context.
  if (!MakeContextCurrent(stub_)) {
    for (const auto& kv : texture_refs_)
      kv.first->ForceContextLost();
  }
  texture_refs_.clear();
}

void VideoFrameFactoryImpl::DropTextureRef(gpu::gles2::TextureRef* ref,
                                           const gpu::SyncToken& token) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  auto it = texture_refs_.find(ref);
  if (it == texture_refs_.end())
    return;
  // If we fail to make the context current, we have to notify the TextureRef
  // so it doesn't try to delete a texture without a context.
  if (!MakeContextCurrent(stub_))
    ref->ForceContextLost();
  texture_refs_.erase(it);
}

void VideoFrameFactoryImpl::OnImageDestructed(void* image_as_void) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  CodecImage* image = static_cast<CodecImage*>(image_as_void);
  base::Erase(images_, image);
  internal::MaybeRenderEarly(&images_);
}

}  // namespace media
