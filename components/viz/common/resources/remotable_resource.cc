// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/common/resources/remotable_resource.h"

#include "build/build_config.h"
#include "components/viz/common/quads/shared_bitmap.h"
#include "ui/gfx/gpu_memory_buffer.h"

namespace viz {
namespace internal {

RemotableResource::RemotableResource(const gfx::Size& size,
                                     Origin origin,
                                     RemotableResourceTextureHint hint,
                                     RemotableResourceType type,
                                     ResourceFormat format,
                                     const gfx::ColorSpace& color_space)
    : locked_for_write(false),
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
      hint(hint),
      type(type),
      format(format),
      color_space(color_space) {
}

RemotableResource::RemotableResource(RemotableResource&& other) = default;
RemotableResource::~RemotableResource() = default;
RemotableResource& RemotableResource::operator=(RemotableResource&& other) =
    default;

void RemotableResource::SetSharedBitmap(SharedBitmap* bitmap) {
  DCHECK(bitmap);
  DCHECK(bitmap->pixels());
  shared_bitmap = bitmap;
  pixels = bitmap->pixels();
  has_shared_bitmap_id = true;
  shared_bitmap_id = bitmap->id();
}

void RemotableResource::SetLocallyUsed() {
  synchronization_state_ = LOCALLY_USED;
  sync_token_.Clear();
}

void RemotableResource::SetSynchronized() {
  synchronization_state_ = SYNCHRONIZED;
}

void RemotableResource::UpdateSyncToken(const gpu::SyncToken& sync_token) {
  DCHECK(IsGpuResourceType(type));
  // An empty sync token may be used if commands are guaranteed to have run on
  // the gpu process or in case of context loss.
  sync_token_ = sync_token;
  synchronization_state_ = sync_token.HasData() ? NEEDS_WAIT : SYNCHRONIZED;
}

int8_t* RemotableResource::GetSyncTokenData() {
  return sync_token_.GetData();
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
