// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/common/resources/remotable_resource.h"

#include "build/build_config.h"
#include "ui/gfx/gpu_memory_buffer.h"

namespace viz {
namespace internal {

RemotableResource::RemotableResource(GLuint texture_id,
                                     const gfx::Size& size,
                                     Origin origin,
                                     GLenum target,
                                     GLenum filter,
                                     RemotableResourceTextureHint hint,
                                     RemotableResourceType type,
                                     ResourceFormat format)
    : child_id(0),
      gl_id(texture_id),
      pixels(nullptr),
      lock_for_read_count(0),
      imported_count(0),
      exported_count(0),
      locked_for_write(false),
      lost(false),
      marked_for_deletion(false),
      allocated(false),
      read_lock_fences_enabled(false),
      has_shared_bitmap_id(false),
      is_overlay_candidate(false),
#if defined(OS_ANDROID)
      is_backed_by_surface_texture(false),
      wants_promotion_hint(false),
#endif
      size(size),
      origin(origin),
      target(target),
      original_filter(filter),
      filter(filter),
      min_filter(filter),
      image_id(0),
      hint(hint),
      type(type),
      usage(gfx::BufferUsage::GPU_READ_CPU_READ_WRITE),
      buffer_format(gfx::BufferFormat::RGBA_8888),
      format(format),
      shared_bitmap(nullptr) {
}

RemotableResource::RemotableResource(uint8_t* pixels,
                                     SharedBitmap* bitmap,
                                     const gfx::Size& size,
                                     Origin origin,
                                     GLenum filter)
    : child_id(0),
      gl_id(0),
      pixels(pixels),
      lock_for_read_count(0),
      imported_count(0),
      exported_count(0),
      locked_for_write(false),
      lost(false),
      marked_for_deletion(false),
      allocated(false),
      read_lock_fences_enabled(false),
      has_shared_bitmap_id(!!bitmap),
      is_overlay_candidate(false),
#if defined(OS_ANDROID)
      is_backed_by_surface_texture(false),
      wants_promotion_hint(false),
#endif
      size(size),
      origin(origin),
      target(0),
      original_filter(filter),
      filter(filter),
      min_filter(filter),
      image_id(0),
      hint(RemotableResourceTextureHint::kImmutable),
      type(RemotableResourceType::kBitmap),
      buffer_format(gfx::BufferFormat::RGBA_8888),
      format(RGBA_8888),
      shared_bitmap(bitmap) {
  DCHECK(origin == DELEGATED || pixels);
  if (bitmap)
    shared_bitmap_id = bitmap->id();
}

RemotableResource::RemotableResource(const SharedBitmapId& bitmap_id,
                                     const gfx::Size& size,
                                     Origin origin,
                                     GLenum filter)
    : child_id(0),
      gl_id(0),
      pixels(nullptr),
      lock_for_read_count(0),
      imported_count(0),
      exported_count(0),
      locked_for_write(false),
      lost(false),
      marked_for_deletion(false),
      allocated(false),
      read_lock_fences_enabled(false),
      has_shared_bitmap_id(true),
      is_overlay_candidate(false),
#if defined(OS_ANDROID)
      is_backed_by_surface_texture(false),
      wants_promotion_hint(false),
#endif
      size(size),
      origin(origin),
      target(0),
      original_filter(filter),
      filter(filter),
      min_filter(filter),
      image_id(0),
      hint(RemotableResourceTextureHint::kImmutable),
      type(RemotableResourceType::kBitmap),
      buffer_format(gfx::BufferFormat::RGBA_8888),
      format(RGBA_8888),
      shared_bitmap_id(bitmap_id),
      shared_bitmap(nullptr) {
}

RemotableResource::RemotableResource(RemotableResource&& other) = default;
RemotableResource::~RemotableResource() = default;
RemotableResource& RemotableResource::operator=(RemotableResource&& other) =
    default;

void RemotableResource::SetMailbox(const TextureMailbox& mailbox) {
  DCHECK(!mailbox.sync_token().HasData() || IsGpuResourceType(type));
  mailbox_ = mailbox;
  synchronization_state_ =
      mailbox.sync_token().HasData() ? NEEDS_WAIT : SYNCHRONIZED;
}

void RemotableResource::SetLocallyUsed() {
  synchronization_state_ = LOCALLY_USED;
  mailbox_.set_sync_token(gpu::SyncToken());
}

void RemotableResource::SetSynchronized() {
  synchronization_state_ = SYNCHRONIZED;
}

void RemotableResource::UpdateSyncToken(const gpu::SyncToken& sync_token) {
  DCHECK(IsGpuResourceType(type));
  // An empty sync token may be used if commands are guaranteed to have run on
  // the gpu process or in case of context loss.
  mailbox_.set_sync_token(sync_token);
  synchronization_state_ = sync_token.HasData() ? NEEDS_WAIT : SYNCHRONIZED;
}

int8_t* RemotableResource::GetSyncTokenData() {
  return mailbox_.GetSyncTokenData();
}

bool RemotableResource::ShouldWaitSyncToken() const {
  return synchronization_state_ == NEEDS_WAIT;
}

void RemotableResource::SetGenerateMipmap() {
  DCHECK(IsGpuResourceType(type));
  DCHECK_EQ(target, static_cast<GLenum>(GL_TEXTURE_2D));
  DCHECK(hint & RemotableResourceTextureHint::kMipmap);
  DCHECK(!gpu_memory_buffer);
  mipmap_state = GENERATE;
}

}  // namespace internal
}  // namespace viz
