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

// GpuSide is an implementation detail of VideoFrameFactoryImpl. It
// may be created on any thread but only accessed on the gpu thread thereafter.
class VideoFrameFactoryImpl::GpuSide
    : public gpu::GpuCommandBufferStub::DestructionObserver {
 public:
  GpuSide();
  ~GpuSide() override;

  scoped_refptr<SurfaceTextureGLOwner> Initialize(
      VideoFrameFactory::GetStubCb get_stub_cb);

  // Creates and returns a VideoFrame with its ReleaseMailboxCB.
  FrameAndReleaseCb CreateVideoFrame(
      std::unique_ptr<CodecOutputBuffer> output_buffer,
      scoped_refptr<SurfaceTextureGLOwner> surface_texture,
      base::TimeDelta timestamp,
      gfx::Size natural_size);

 private:
  // Creates a TextureRef and VideoFrame.
  void CreateVideoFrameInternal(
      std::unique_ptr<CodecOutputBuffer> output_buffer,
      scoped_refptr<SurfaceTextureGLOwner> surface_texture,
      base::TimeDelta timestamp,
      gfx::Size natural_size,
      scoped_refptr<VideoFrame>* video_frame_out,
      scoped_refptr<gpu::gles2::TextureRef>* texture_ref_out);

  void OnWillDestroyStub() override;

  // Clears |texture_refs_|. Makes the gl context current.
  void ClearTextureRefs();

  // Removes |ref| from texture_refs_. Makes the gl context current.
  // |token| is ignored because MojoVideoDecoderService guarantees that it has
  // already passed by the time we get the callback.
  void DropTextureRef(gpu::gles2::TextureRef* ref, const gpu::SyncToken& token);

  // Removes |image| from |images_|.
  void OnImageDestructed(CodecImage* image);

  // Outstanding images that should be considered for early rendering.
  std::vector<CodecImage*> images_;

  // Outstanding TextureRefs that are still referenced by a mailbox VideoFrame.
  // They're kept alive until their mailboxes are released (or |this| is
  // destructed).
  std::map<gpu::gles2::TextureRef*, scoped_refptr<gpu::gles2::TextureRef>>
      texture_refs_;
  gpu::GpuCommandBufferStub* stub_;

  // A helper for creating textures. Only valid while |stub_| is valid.
  std::unique_ptr<GLES2DecoderHelper> decoder_helper_;
  base::WeakPtrFactory<GpuSide> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(GpuSide);
};

VideoFrameFactoryImpl::VideoFrameFactoryImpl(
    scoped_refptr<base::SingleThreadTaskRunner> gpu_task_runner,
    GetStubCb get_stub_cb)
    : gpu_task_runner_(std::move(gpu_task_runner)),
      get_stub_cb_(std::move(get_stub_cb)),
      weak_factory_(this) {}

VideoFrameFactoryImpl::~VideoFrameFactoryImpl() {
  if (gpu_side_)
    gpu_task_runner_->DeleteSoon(FROM_HERE, gpu_side_.release());
}

void VideoFrameFactoryImpl::Initialize(InitCb init_cb) {
  DCHECK(!gpu_side_);
  gpu_side_ = base::MakeUnique<GpuSide>();
  base::PostTaskAndReplyWithResult(
      gpu_task_runner_.get(), FROM_HERE,
      base::Bind(&GpuSide::Initialize, base::Unretained(gpu_side_.get()),
                 get_stub_cb_),
      base::Bind(&VideoFrameFactoryImpl::OnInitializeDone,
                 weak_factory_.GetWeakPtr(), std::move(init_cb)));
}

void VideoFrameFactoryImpl::OnInitializeDone(
    InitCb init_cb,
    scoped_refptr<SurfaceTextureGLOwner> surface_texture) {
  init_cb.Run(std::move(surface_texture));
}

void VideoFrameFactoryImpl::CreateVideoFrame(
    std::unique_ptr<CodecOutputBuffer> output_buffer,
    scoped_refptr<SurfaceTextureGLOwner> surface_texture,
    base::TimeDelta timestamp,
    gfx::Size natural_size,
    OutputWithReleaseMailboxCB output_cb) {
  base::PostTaskAndReplyWithResult(
      gpu_task_runner_.get(), FROM_HERE,
      base::Bind(&GpuSide::CreateVideoFrame, base::Unretained(gpu_side_.get()),
                 base::Passed(&output_buffer), surface_texture, timestamp,
                 natural_size),
      base::Bind(&VideoFrameFactoryImpl::OnVideoFrameCreated,
                 weak_factory_.GetWeakPtr(), std::move(output_cb)));
}

void VideoFrameFactoryImpl::OnVideoFrameCreated(
    VideoFrameFactory::OutputWithReleaseMailboxCB output_cb,
    FrameAndReleaseCb frame_and_release_cb) {
  if (frame_and_release_cb.first)
    output_cb.Run(frame_and_release_cb.second, frame_and_release_cb.first);
}

void VideoFrameFactoryImpl::RunAfterPendingVideoFrames(base::Closure closure) {
  // Hop through |gpu_task_runner_| to ensure it comes after pending frames.
  gpu_task_runner_->PostTaskAndReply(
      FROM_HERE, base::BindOnce(&base::DoNothing),
      base::Bind(&VideoFrameFactoryImpl::RunClosure, weak_factory_.GetWeakPtr(),
                 std::move(closure)));
}

void VideoFrameFactoryImpl::RunClosure(base::Closure closure) {
  closure.Run();
}

void VideoFrameFactoryImpl::CancelPendingCallbacks() {
  weak_factory_.InvalidateWeakPtrs();
}

VideoFrameFactoryImpl::GpuSide::GpuSide() : weak_factory_(this) {}

VideoFrameFactoryImpl::GpuSide::~GpuSide() {
  if (stub_)
    stub_->RemoveDestructionObserver(this);
  ClearTextureRefs();
}

scoped_refptr<SurfaceTextureGLOwner> VideoFrameFactoryImpl::GpuSide::Initialize(
    VideoFrameFactoryImpl::GetStubCb get_stub_cb) {
  stub_ = get_stub_cb.Run();
  if (!MakeContextCurrent(stub_))
    return nullptr;
  stub_->AddDestructionObserver(this);
  decoder_helper_ = GLES2DecoderHelper::Create(stub_->decoder());
  return SurfaceTextureGLOwnerImpl::Create();
}

FrameAndReleaseCb VideoFrameFactoryImpl::GpuSide::CreateVideoFrame(
    std::unique_ptr<CodecOutputBuffer> output_buffer,
    scoped_refptr<SurfaceTextureGLOwner> surface_texture,
    base::TimeDelta timestamp,
    gfx::Size natural_size) {
  scoped_refptr<VideoFrame> frame;
  scoped_refptr<gpu::gles2::TextureRef> texture_ref;
  CreateVideoFrameInternal(std::move(output_buffer), std::move(surface_texture),
                           timestamp, natural_size, &frame, &texture_ref);
  if (!frame || !texture_ref)
    return FrameAndReleaseCb();

  // TODO(sandersd, watk): The VideoFrame release callback will not be called
  // after MojoVideoDecoderService is destructed, so we have to release all
  // our TextureRefs when |this| is destructed. This can unback outstanding
  // VideoFrames (e.g., the current frame when the player is suspended). The
  // release callback lifetime should be separate from MCVD or
  // MojoVideoDecoderService (http://crbug.com/737220).
  texture_refs_[texture_ref.get()] = texture_ref;
  auto release_cb = BindToCurrentLoop(
      base::Bind(&GpuSide::DropTextureRef, weak_factory_.GetWeakPtr(),
                 base::Unretained(texture_ref.get())));
  return FrameAndReleaseCb(std::move(frame), std::move(release_cb));
}

void VideoFrameFactoryImpl::GpuSide::CreateVideoFrameInternal(
    std::unique_ptr<CodecOutputBuffer> output_buffer,
    scoped_refptr<SurfaceTextureGLOwner> surface_texture,
    base::TimeDelta timestamp,
    gfx::Size natural_size,
    scoped_refptr<VideoFrame>* video_frame_out,
    scoped_refptr<gpu::gles2::TextureRef>* texture_ref_out) {
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
  // Check that we can create a VideoFrame for this config before creating the
  // TextureRef so that we don't have to clean up the TextureRef if creating the
  // frame fails.
  if (!VideoFrame::IsValidConfig(PIXEL_FORMAT_ARGB, VideoFrame::STORAGE_OPAQUE,
                                 size, visible_rect, natural_size)) {
    return;
  }

  // Create a new Texture.
  auto texture_ref = decoder_helper_->CreateTexture(
      GL_TEXTURE_EXTERNAL_OES, GL_RGBA, size.width(), size.height(), GL_RGBA,
      GL_UNSIGNED_BYTE);

  // Create a new CodecImage to back the texture and try to render it early.
  auto image = make_scoped_refptr(new CodecImage(
      std::move(output_buffer), surface_texture,
      base::Bind(&GpuSide::OnImageDestructed, weak_factory_.GetWeakPtr())));
  images_.push_back(image.get());
  internal::MaybeRenderEarly(&images_);

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

  // Note: The pixel format doesn't matter.
  auto frame = VideoFrame::WrapNativeTextures(
      PIXEL_FORMAT_ARGB, mailbox_holders, VideoFrame::ReleaseMailboxCB(), size,
      visible_rect, natural_size, timestamp);

  *video_frame_out = std::move(frame);
  *texture_ref_out = std::move(texture_ref);
}

void VideoFrameFactoryImpl::GpuSide::OnWillDestroyStub() {
  DCHECK(stub_);
  ClearTextureRefs();
  stub_ = nullptr;
  decoder_helper_ = nullptr;
}

void VideoFrameFactoryImpl::GpuSide::ClearTextureRefs() {
  DCHECK(stub_ || texture_refs_.empty());
  // If we fail to make the context current, we have to notify the TextureRefs
  // so they don't try to delete textures without a context.
  if (!MakeContextCurrent(stub_)) {
    for (const auto& kv : texture_refs_)
      kv.first->ForceContextLost();
  }
  texture_refs_.clear();
}

void VideoFrameFactoryImpl::GpuSide::DropTextureRef(
    gpu::gles2::TextureRef* ref,
    const gpu::SyncToken& token) {
  auto it = texture_refs_.find(ref);
  if (it == texture_refs_.end())
    return;
  // If we fail to make the context current, we have to notify the TextureRef
  // so it doesn't try to delete a texture without a context.
  if (!MakeContextCurrent(stub_))
    ref->ForceContextLost();
  texture_refs_.erase(it);
}

void VideoFrameFactoryImpl::GpuSide::OnImageDestructed(CodecImage* image) {
  base::Erase(images_, image);
  internal::MaybeRenderEarly(&images_);
}

}  // namespace media
