// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/exo/buffer.h"

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES2/gl2extchromium.h>
#include <stdint.h>

#include <algorithm>
#include <utility>

#include "base/callback_helpers.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "base/trace_event/trace_event.h"
#include "base/trace_event/trace_event_argument.h"
#include "components/exo/layer_tree_frame_sink_holder.h"
#include "components/viz/common/gpu/context_provider.h"
#include "components/viz/common/quads/single_release_callback.h"
#include "components/viz/common/quads/texture_mailbox.h"
#include "components/viz/common/resources/resource_format.h"
#include "gpu/command_buffer/client/context_support.h"
#include "gpu/command_buffer/client/gles2_interface.h"
#include "ui/aura/env.h"
#include "ui/compositor/compositor.h"
#include "ui/gfx/gpu_memory_buffer.h"

namespace exo {
namespace {

// The amount of time before we wait for release queries using
// GetQueryObjectuivEXT(GL_QUERY_RESULT_EXT).
const int kWaitForReleaseDelayMs = 500;

GLenum GLInternalFormat(gfx::BufferFormat format) {
  const GLenum kGLInternalFormats[] = {
      GL_ATC_RGBA_INTERPOLATED_ALPHA_AMD,  // ATC
      GL_COMPRESSED_RGB_S3TC_DXT1_EXT,     // ATCIA
      GL_COMPRESSED_RGB_S3TC_DXT1_EXT,     // DXT1
      GL_COMPRESSED_RGBA_S3TC_DXT5_EXT,    // DXT5
      GL_ETC1_RGB8_OES,                    // ETC1
      GL_R8_EXT,                           // R_8
      GL_R16_EXT,                          // R_16
      GL_RG8_EXT,                          // RG_88
      GL_RGB,                              // BGR_565
      GL_RGBA,                             // RGBA_4444
      GL_RGB,                              // RGBX_8888
      GL_RGBA,                             // RGBA_8888
      GL_RGB,                              // BGRX_8888
      GL_BGRA_EXT,                         // BGRA_8888
      GL_RGBA,                             // RGBA_F16
      GL_RGB_YCRCB_420_CHROMIUM,           // YVU_420
      GL_RGB_YCBCR_420V_CHROMIUM,          // YUV_420_BIPLANAR
      GL_RGB_YCBCR_422_CHROMIUM,           // UYVY_422
  };
  static_assert(arraysize(kGLInternalFormats) ==
                    (static_cast<int>(gfx::BufferFormat::LAST) + 1),
                "BufferFormat::LAST must be last value of kGLInternalFormats");

  DCHECK(format <= gfx::BufferFormat::LAST);
  return kGLInternalFormats[static_cast<int>(format)];
}

unsigned CreateGLTexture(gpu::gles2::GLES2Interface* gles2, GLenum target) {
  unsigned texture_id = 0;
  gles2->GenTextures(1, &texture_id);
  gles2->ActiveTexture(GL_TEXTURE0);
  gles2->BindTexture(target, texture_id);
  gles2->TexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  gles2->TexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  gles2->TexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  gles2->TexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  return texture_id;
}

void CreateGLTextureMailbox(gpu::gles2::GLES2Interface* gles2,
                            unsigned texture_id,
                            GLenum target,
                            gpu::Mailbox* mailbox) {
  gles2->ActiveTexture(GL_TEXTURE0);
  gles2->BindTexture(target, texture_id);
  gles2->GenMailboxCHROMIUM(mailbox->name);
  gles2->ProduceTextureCHROMIUM(target, mailbox->name);
}

}  // namespace

////////////////////////////////////////////////////////////////////////////////
// Buffer::Texture

// Encapsulates the state and logic needed to bind a buffer to a GLES2 texture.
class Buffer::Texture : public base::RefCounted<Buffer::Texture>,
                        public ui::ContextFactoryObserver {
 public:
  Texture(ui::ContextFactory* context_factory,
          viz::ContextProvider* context_provider);
  Texture(ui::ContextFactory* context_factory,
          viz::ContextProvider* context_provider,
          gfx::GpuMemoryBuffer* gpu_memory_buffer,
          unsigned texture_target,
          unsigned query_type);

  // Overridden from ui::ContextFactoryObserver:
  void OnLostResources() override;

  // Returns true if GLES2 resources for texture have been lost.
  bool IsLost();

  // Issue GL WatiSyncToken command on texture context.
  void WaitSyncToken(const gpu::SyncToken& sync_token);

  // Allow texture to be reused.
  void ReleaseTex(const gpu::SyncToken& sync_token, bool is_lost);

  // Binds the contents referenced by |image_id_| to the texture returned by
  // mailbox(). Returns a sync token that can be used when accessing texture
  // from a different context.
  gpu::SyncToken BindTexImage();

  // Releases the contents referenced by |image_id_| after |sync_token| has
  // passed and runs |callback| when completed.
  void ReleaseTexImage(const base::Closure& callback,
                       const gpu::SyncToken& sync_token,
                       bool is_lost);

  // Copy the contents of texture to |destination| and runs |callback| when
  // completed. Returns a sync token that can be used when accessing texture
  // from a different context.
  gpu::SyncToken CopyTexImage(const scoped_refptr<Texture>& destination,
                              const base::Closure& callback);

  // Returns the mailbox for this texture.
  gpu::Mailbox mailbox() const { return mailbox_; }

 private:
  friend class base::RefCounted<Buffer::Texture>;
  ~Texture() override;

  void DestroyResources();
  void ReleaseWhenQueryResultIsAvailable(const base::Closure& callback);
  void Released();
  void ScheduleWaitForRelease(base::TimeDelta delay);
  void WaitForRelease();

  gfx::GpuMemoryBuffer* const gpu_memory_buffer_;
  ui::ContextFactory* context_factory_;
  scoped_refptr<viz::ContextProvider> context_provider_;
  const unsigned texture_target_;
  const unsigned query_type_;
  const GLenum internalformat_;
  unsigned image_id_ = 0;
  unsigned query_id_ = 0;
  unsigned texture_id_ = 0;
  gpu::Mailbox mailbox_;
  base::Closure release_callback_;
  base::TimeTicks wait_for_release_time_;
  bool wait_for_release_pending_ = false;
  base::WeakPtrFactory<Texture> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(Texture);
};

Buffer::Texture::Texture(ui::ContextFactory* context_factory,
                         viz::ContextProvider* context_provider)
    : gpu_memory_buffer_(nullptr),
      context_factory_(context_factory),
      context_provider_(context_provider),
      texture_target_(GL_TEXTURE_2D),
      query_type_(GL_COMMANDS_COMPLETED_CHROMIUM),
      internalformat_(GL_RGBA),
      weak_ptr_factory_(this) {
  gpu::gles2::GLES2Interface* gles2 = context_provider_->ContextGL();
  texture_id_ = CreateGLTexture(gles2, texture_target_);
  // Generate a crypto-secure random mailbox name.
  CreateGLTextureMailbox(gles2, texture_id_, texture_target_, &mailbox_);
  // Provides a notification when |context_provider_| is lost.
  context_factory_->AddObserver(this);
}

Buffer::Texture::Texture(ui::ContextFactory* context_factory,
                         viz::ContextProvider* context_provider,
                         gfx::GpuMemoryBuffer* gpu_memory_buffer,
                         unsigned texture_target,
                         unsigned query_type)
    : gpu_memory_buffer_(gpu_memory_buffer),
      context_factory_(context_factory),
      context_provider_(context_provider),
      texture_target_(texture_target),
      query_type_(query_type),
      internalformat_(GLInternalFormat(gpu_memory_buffer->GetFormat())),
      weak_ptr_factory_(this) {
  gpu::gles2::GLES2Interface* gles2 = context_provider_->ContextGL();
  gfx::Size size = gpu_memory_buffer->GetSize();
  image_id_ =
      gles2->CreateImageCHROMIUM(gpu_memory_buffer->AsClientBuffer(),
                                 size.width(), size.height(), internalformat_);
  DLOG_IF(WARNING, !image_id_) << "Failed to create GLImage";

  gles2->GenQueriesEXT(1, &query_id_);
  texture_id_ = CreateGLTexture(gles2, texture_target_);
  // Provides a notification when |context_provider_| is lost.
  context_factory_->AddObserver(this);
}

Buffer::Texture::~Texture() {
  DestroyResources();
  if (context_provider_)
    context_factory_->RemoveObserver(this);
}

void Buffer::Texture::OnLostResources() {
  DestroyResources();
  context_factory_->RemoveObserver(this);
  context_provider_ = nullptr;
  context_factory_ = nullptr;
}

bool Buffer::Texture::IsLost() {
  if (context_provider_) {
    gpu::gles2::GLES2Interface* gles2 = context_provider_->ContextGL();
    return gles2->GetGraphicsResetStatusKHR() != GL_NO_ERROR;
  }
  return true;
}

void Buffer::Texture::WaitSyncToken(const gpu::SyncToken& sync_token) {
  if (context_provider_ && sync_token.HasData()) {
    gpu::gles2::GLES2Interface* gles2 = context_provider_->ContextGL();
    gles2->WaitSyncTokenCHROMIUM(sync_token.GetConstData());
  }
}

void Buffer::Texture::ReleaseTex(const gpu::SyncToken& sync_token,
                                 bool is_lost) {
  WaitSyncToken(sync_token);
}

gpu::SyncToken Buffer::Texture::BindTexImage() {
  gpu::SyncToken sync_token;
  if (context_provider_) {
    gpu::gles2::GLES2Interface* gles2 = context_provider_->ContextGL();
    gles2->ActiveTexture(GL_TEXTURE0);
    gles2->BindTexture(texture_target_, texture_id_);
    DCHECK_NE(image_id_, 0u);
    gles2->BindTexImage2DCHROMIUM(texture_target_, image_id_);
    // Generate a crypto-secure random mailbox name if not already done.
    if (mailbox_.IsZero())
      CreateGLTextureMailbox(gles2, texture_id_, texture_target_, &mailbox_);
    // Create and return a sync token that can be used to ensure that the
    // BindTexImage2DCHROMIUM call is processed before issuing any commands
    // that will read from the texture on a different context.
    uint64_t fence_sync = gles2->InsertFenceSyncCHROMIUM();
    gles2->OrderingBarrierCHROMIUM();
    gles2->GenUnverifiedSyncTokenCHROMIUM(fence_sync, sync_token.GetData());
    TRACE_EVENT_ASYNC_STEP_INTO0("exo", "BufferInUse", gpu_memory_buffer_,
                                 "bound");
  }
  return sync_token;
}

void Buffer::Texture::ReleaseTexImage(const base::Closure& callback,
                                      const gpu::SyncToken& sync_token,
                                      bool is_lost) {
  WaitSyncToken(sync_token);
  if (context_provider_) {
    gpu::gles2::GLES2Interface* gles2 = context_provider_->ContextGL();
    gles2->ActiveTexture(GL_TEXTURE0);
    gles2->BindTexture(texture_target_, texture_id_);
    DCHECK_NE(query_id_, 0u);
    gles2->BeginQueryEXT(query_type_, query_id_);
    gles2->ReleaseTexImage2DCHROMIUM(texture_target_, image_id_);
    gles2->EndQueryEXT(query_type_);
    // Run callback when query result is available and ReleaseTexImage has
    // been handled if sync token has data and buffer has been used. If buffer
    // was never used then run the callback immediately.
    if (sync_token.HasData()) {
      ReleaseWhenQueryResultIsAvailable(callback);
      return;
    }
  }
  callback.Run();
}

gpu::SyncToken Buffer::Texture::CopyTexImage(
    const scoped_refptr<Texture>& destination,
    const base::Closure& callback) {
  gpu::SyncToken sync_token;
  if (context_provider_) {
    gpu::gles2::GLES2Interface* gles2 = context_provider_->ContextGL();
    gles2->ActiveTexture(GL_TEXTURE0);
    gles2->BindTexture(texture_target_, texture_id_);
    DCHECK_NE(image_id_, 0u);
    gles2->BindTexImage2DCHROMIUM(texture_target_, image_id_);
    gles2->CopyTextureCHROMIUM(texture_id_, 0, destination->texture_target_,
                               destination->texture_id_, 0, internalformat_,
                               GL_UNSIGNED_BYTE, false, false, false);
    DCHECK_NE(query_id_, 0u);
    gles2->BeginQueryEXT(query_type_, query_id_);
    gles2->ReleaseTexImage2DCHROMIUM(texture_target_, image_id_);
    gles2->EndQueryEXT(query_type_);
    // Run callback when query result is available and ReleaseTexImage has
    // been handled.
    ReleaseWhenQueryResultIsAvailable(callback);
    // Create and return a sync token that can be used to ensure that the
    // CopyTextureCHROMIUM call is processed before issuing any commands
    // that will read from the target texture on a different context.
    uint64_t fence_sync = gles2->InsertFenceSyncCHROMIUM();
    gles2->OrderingBarrierCHROMIUM();
    gles2->GenUnverifiedSyncTokenCHROMIUM(fence_sync, sync_token.GetData());
  }
  return sync_token;
}

void Buffer::Texture::DestroyResources() {
  if (context_provider_) {
    gpu::gles2::GLES2Interface* gles2 = context_provider_->ContextGL();
    gles2->DeleteTextures(1, &texture_id_);
    if (query_id_)
      gles2->DeleteQueriesEXT(1, &query_id_);
    if (image_id_)
      gles2->DestroyImageCHROMIUM(image_id_);
  }
}

void Buffer::Texture::ReleaseWhenQueryResultIsAvailable(
    const base::Closure& callback) {
  DCHECK(context_provider_);
  DCHECK(release_callback_.is_null());
  release_callback_ = callback;
  base::TimeDelta wait_for_release_delay =
      base::TimeDelta::FromMilliseconds(kWaitForReleaseDelayMs);
  wait_for_release_time_ = base::TimeTicks::Now() + wait_for_release_delay;
  ScheduleWaitForRelease(wait_for_release_delay);
  TRACE_EVENT_ASYNC_STEP_INTO0("exo", "BufferInUse", gpu_memory_buffer_,
                               "pending_query");
  context_provider_->ContextSupport()->SignalQuery(
      query_id_,
      base::Bind(&Buffer::Texture::Released, weak_ptr_factory_.GetWeakPtr()));
}

void Buffer::Texture::Released() {
  if (!release_callback_.is_null())
    base::ResetAndReturn(&release_callback_).Run();
}

void Buffer::Texture::ScheduleWaitForRelease(base::TimeDelta delay) {
  if (wait_for_release_pending_)
    return;

  wait_for_release_pending_ = true;
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE, base::Bind(&Buffer::Texture::WaitForRelease,
                            weak_ptr_factory_.GetWeakPtr()),
      delay);
}

void Buffer::Texture::WaitForRelease() {
  DCHECK(wait_for_release_pending_);
  wait_for_release_pending_ = false;

  if (release_callback_.is_null())
    return;

  base::TimeTicks current_time = base::TimeTicks::Now();
  if (current_time < wait_for_release_time_) {
    ScheduleWaitForRelease(wait_for_release_time_ - current_time);
    return;
  }

  base::Closure callback = base::ResetAndReturn(&release_callback_);

  if (context_provider_) {
    TRACE_EVENT0("exo", "Buffer::Texture::WaitForQueryResult");

    // We need to wait for the result to be available. Getting the result of
    // the query implies waiting for it to become available. The actual result
    // is unimportant and also not well defined.
    unsigned result = 0;
    gpu::gles2::GLES2Interface* gles2 = context_provider_->ContextGL();
    gles2->GetQueryObjectuivEXT(query_id_, GL_QUERY_RESULT_EXT, &result);
  }

  callback.Run();
}

////////////////////////////////////////////////////////////////////////////////
// Buffer, public:

Buffer::Buffer(std::unique_ptr<gfx::GpuMemoryBuffer> gpu_memory_buffer)
    : gpu_memory_buffer_(std::move(gpu_memory_buffer)),
      texture_target_(GL_TEXTURE_2D),
      query_type_(GL_COMMANDS_COMPLETED_CHROMIUM),
      use_zero_copy_(true),
      is_overlay_candidate_(false) {}

Buffer::Buffer(std::unique_ptr<gfx::GpuMemoryBuffer> gpu_memory_buffer,
               unsigned texture_target,
               unsigned query_type,
               bool use_zero_copy,
               bool is_overlay_candidate)
    : gpu_memory_buffer_(std::move(gpu_memory_buffer)),
      texture_target_(texture_target),
      query_type_(query_type),
      use_zero_copy_(use_zero_copy),
      is_overlay_candidate_(is_overlay_candidate) {}

Buffer::~Buffer() {}

bool Buffer::ProduceTransferableResource(
    LayerTreeFrameSinkHolder* layer_tree_frame_sink_holder,
    bool secure_output_only,
    bool client_usage) {
  TRACE_EVENT0("exo", "Buffer::ProduceTransferableResource");

  DCHECK(is_attached());
  DCHECK(release_contents_callback_.IsCancelled() || !client_usage)
      << "Producing a texture mailbox for a buffer that has not been released";

  // If textures are lost, destroy them to ensure that we create new ones below.
  if (contents_texture_ && contents_texture_->IsLost())
    contents_texture_ = nullptr;
  if (texture_ && texture_->IsLost())
    texture_ = nullptr;

  ui::ContextFactory* context_factory =
      aura::Env::GetInstance()->context_factory();
  // Note: This can fail if GPU acceleration has been disabled.
  scoped_refptr<viz::ContextProvider> context_provider =
      context_factory->SharedMainThreadContextProvider();
  if (!context_provider) {
    DLOG(WARNING) << "Failed to acquire a context provider";
    resource_.id = 0;
    resource_.size = gfx::Size();
    return false;
  }

  resource_.id = layer_tree_frame_sink_holder->AllocateResourceId();
  resource_.format = viz::RGBA_8888;
  resource_.filter = GL_LINEAR;
  resource_.size = gpu_memory_buffer_->GetSize();

  // Create a new image texture for |gpu_memory_buffer_| with |texture_target_|
  // if one doesn't already exist. The contents of this buffer are copied to
  // |texture| using a call to CopyTexImage.
  if (!contents_texture_) {
    contents_texture_ = base::MakeRefCounted<Texture>(
        context_factory, context_provider.get(), gpu_memory_buffer_.get(),
        texture_target_, query_type_);
  }

  if (release_contents_callback_.IsCancelled())
    TRACE_EVENT_ASYNC_BEGIN0("exo", "BufferInUse", gpu_memory_buffer_.get());

  // Cancel pending contents release callback.
  release_contents_callback_.Reset(
      base::Bind(&Buffer::ReleaseContents, base::Unretained(this)));

  // Zero-copy means using the contents texture directly.
  if (use_zero_copy_) {
    // This binds the latest contents of this buffer to |contents_texture_|.
    gpu::SyncToken sync_token = contents_texture_->BindTexImage();
    resource_.mailbox_holder = gpu::MailboxHolder(contents_texture_->mailbox(),
                                                  sync_token, texture_target_);
    resource_.is_overlay_candidate = is_overlay_candidate_;
    resource_.buffer_format = gpu_memory_buffer_->GetFormat();
    RegisterTransferableResource(layer_tree_frame_sink_holder);

    return true;
  }

  // Create a mailbox texture that we copy the buffer contents to.
  if (!texture_) {
    texture_ =
        base::MakeRefCounted<Texture>(context_factory, context_provider.get());
  }

  // Copy the contents of |contents_texture_| to |texture_| and produce a
  // texture mailbox from the result in |texture_|. The contents texture will
  // be released when copy has completed.
  gpu::SyncToken sync_token = contents_texture_->CopyTexImage(
      texture_,
      base::Bind(&Buffer::ReleaseContentsTexture, AsWeakPtr(),
                 contents_texture_, release_contents_callback_.callback(),
                 gpu::SyncToken(), false));
  resource_.mailbox_holder =
      gpu::MailboxHolder(texture_->mailbox(), sync_token, GL_TEXTURE_2D);
  resource_.is_overlay_candidate = false;

  RegisterTransferableResource(layer_tree_frame_sink_holder);
  return true;
}

void Buffer::RegisterTransferableResource(
    LayerTreeFrameSinkHolder* layer_tree_frame_sink_holder) {
  DCHECK(resource_.id);
  DCHECK(!resource_registered_);

  if (use_zero_copy_) {
    // The contents texture will be released when no longer used by the
    // compositor.
    layer_tree_frame_sink_holder->SetResourceReleaseCallback(
        resource_.id,
        base::Bind(&Buffer::ReleaseContentsTexture, AsWeakPtr(),
                   contents_texture_, release_contents_callback_.callback()));

  } else {
    // The mailbox texture will be released when no longer used by the
    // compositor.
    layer_tree_frame_sink_holder->SetResourceReleaseCallback(
        resource_.id,
        base::Bind(&Buffer::ReleaseTexture, AsWeakPtr(), texture_));
  }
  resource_registered_ = true;
}

void Buffer::OnAttach() {
  DLOG_IF(WARNING, attach_count_)
      << "Reattaching a buffer that is already attached to another surface.";
  ++attach_count_;
}

void Buffer::OnDetach() {
  DCHECK_GT(attach_count_, 0u);
  --attach_count_;

  // Release buffer if no longer attached to a surface and content has been
  // released.
  if (!is_attached()) {
    if (release_contents_callback_.IsCancelled()) {
      Release();
    } else if (!is_resource_registered() && use_zero_copy_) {
      contents_texture_->ReleaseTexImage(release_contents_callback_.callback(),
                                         gpu::SyncToken(), false);
    }
  }
}

gfx::Size Buffer::GetSize() const {
  return gpu_memory_buffer_->GetSize();
}

gfx::BufferFormat Buffer::GetFormat() const {
  return gpu_memory_buffer_->GetFormat();
}

std::unique_ptr<base::trace_event::TracedValue> Buffer::AsTracedValue() const {
  std::unique_ptr<base::trace_event::TracedValue> value(
      new base::trace_event::TracedValue());
  gfx::Size size = gpu_memory_buffer_->GetSize();
  value->SetInteger("width", size.width());
  value->SetInteger("height", size.height());
  value->SetInteger("format",
                    static_cast<int>(gpu_memory_buffer_->GetFormat()));
  return value;
}

////////////////////////////////////////////////////////////////////////////////
// Buffer, private:

void Buffer::Release() {
  TRACE_EVENT_ASYNC_END0("exo", "BufferInUse", gpu_memory_buffer_.get());

  // Run release callback to notify the client that buffer has been released.
  if (!release_callback_.is_null())
    release_callback_.Run();
}

void Buffer::ReleaseTexture(const scoped_refptr<Texture>& texture,
                            const gpu::SyncToken& sync_token,
                            bool is_lost) {
  DCHECK(!use_zero_copy_);
  if (texture == texture_) {
    // If the buffer is not attached from a surface, the |texture_| could be
    // sent to the compositor again, so we should not release it.
    if (!is_attached())
      texture_->ReleaseTex(sync_token, is_lost);
    resource_registered_ = false;
  } else {
    // In this case, the current |texture_| has been replaced, so we can release
    // the old |texture| safely.
    texture->ReleaseTex(sync_token, is_lost);
  }
}

void Buffer::ReleaseContentsTexture(const scoped_refptr<Texture>& texture,
                                    const base::Closure& callback,
                                    const gpu::SyncToken& sync_token,
                                    bool is_lost) {
  DCHECK_EQ(texture, contents_texture_);
  if (use_zero_copy_) {
    if (texture == contents_texture_) {
      // If zero copy is used, we send the |contents_texture_| to compositor
      // directly, so if the buffer is not detached from a surface, the
      // |contents_texture_| could be sent to the compositor again, so should
      // not release it, and we just need issue a WaitSyncToken GL command.
      if (!is_attached()) {
        contents_texture_->ReleaseTexImage(callback, sync_token, is_lost);
      } else {
        contents_texture_->WaitSyncToken(sync_token);
      }
      resource_registered_ = false;
    } else {
      // In this case, the current |context_texture_| has been replaced, so we
      // can release the old |texture| safely.
      texture->ReleaseTexImage(callback, sync_token, is_lost);
    }
  } else {
    // If zero copy is not used, at this moment, the contents of the
    // |contents_texture_| should have been copied to |texture_|, so we can
    // release the |texture| safely.
    texture->ReleaseTex(sync_token, is_lost);
    callback.Run();
  }
}

void Buffer::ReleaseContents() {
  TRACE_EVENT0("exo", "Buffer::ReleaseContents");

  // Cancel callback to indicate that buffer has been released.
  release_contents_callback_.Cancel();

  if (is_attached()) {
    // Do not release buffer until the buffer is detach from the surface.
    TRACE_EVENT_ASYNC_STEP_INTO0("exo", "BufferInUse", gpu_memory_buffer_.get(),
                                 "attached");
  } else {
    // Release buffer if not attached to surface.
    Release();
  }
}

}  // namespace exo
