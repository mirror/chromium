// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_COMMON_RESOURCES_REMOTABLE_RESOURCE_H_
#define COMPONENTS_VIZ_COMMON_RESOURCES_REMOTABLE_RESOURCE_H_

#include <memory>

#include "build/build_config.h"
#include "components/viz/common/quads/release_callback.h"
#include "components/viz/common/quads/shared_bitmap.h"
#include "components/viz/common/quads/texture_mailbox.h"
#include "components/viz/common/resources/remotable_resource_fence.h"
#include "components/viz/common/resources/remotable_resource_texture_hint.h"
#include "components/viz/common/resources/remotable_resource_type.h"
#include "components/viz/common/resources/resource_format.h"
#include "components/viz/common/resources/resource_id.h"
#include "components/viz/common/viz_common_export.h"
#include "third_party/khronos/GLES2/gl2.h"
#include "ui/gfx/buffer_types.h"
#include "ui/gfx/color_space.h"
#include "ui/gfx/geometry/size.h"

namespace gfx {
class GpuMemoryBuffer;
}

namespace gpu {
struct SyncToken;
}

namespace viz {
namespace internal {

// The data structure used to track state of Gpu and Software-based
// resources both in the client and the service, for resources transferred
// between the two. This is an implementation detail of the resource tracking
// for client and service libraries and should not be used directly from
// external client code.
struct VIZ_COMMON_EXPORT RemotableResource {
  enum Origin { INTERNAL, EXTERNAL, DELEGATED };
  enum SynchronizationState {
    // The LOCALLY_USED state is the state each resource defaults to when
    // constructed or modified or read. This state indicates that the
    // resource has not been properly synchronized and it would be an error
    // to send this resource to a parent, child, or client.
    LOCALLY_USED,

    // The NEEDS_WAIT state is the state that indicates a resource has been
    // modified but it also has an associated sync token assigned to it.
    // The sync token has not been waited on with the local context. When
    // a sync token arrives from an external resource (such as a child or
    // parent), it is automatically initialized as NEEDS_WAIT as well
    // since we still need to wait on it before the resource is synchronized
    // on the current context. It is an error to use the resource locally for
    // reading or writing if the resource is in this state.
    NEEDS_WAIT,

    // The SYNCHRONIZED state indicates that the resource has been properly
    // synchronized locally. This can either synchronized externally (such
    // as the case of software rasterized bitmaps), or synchronized
    // internally using a sync token that has been waited upon. In the
    // former case where the resource was synchronized externally, a
    // corresponding sync token will not exist. In the latter case which was
    // synchronized from the NEEDS_WAIT state, a corresponding sync token will
    // exist which is assocaited with the resource. This sync token is still
    // valid and still associated with the resource and can be passed as an
    // external resource for others to wait on.
    SYNCHRONIZED,
  };
  enum MipmapState { INVALID, GENERATE, VALID };

  static bool IsGpuResourceType(RemotableResourceType type) {
    return type != RemotableResourceType::kBitmap;
  }

  // Makes a Gpu texture-backed or GpuMemoryBuffer-backed resource.
  RemotableResource(GLuint texture_id,
                    const gfx::Size& size,
                    Origin origin,
                    GLenum target,
                    GLenum filter,
                    RemotableResourceTextureHint hint,
                    RemotableResourceType type,
                    ResourceFormat format);
  // Makes a Software bitmap-backed resource where the memory is owned
  // externally. The SharedBitmap is not present if the backing is not in
  // shared memory.
  RemotableResource(uint8_t* pixels,
                    SharedBitmap* bitmap,
                    const gfx::Size& size,
                    Origin origin,
                    GLenum filter);
  // Makes a Software bitmap-backed resource where the SharedBitmap is owned
  // internally.
  RemotableResource(const SharedBitmapId& bitmap_id,
                    const gfx::Size& size,
                    Origin origin,
                    GLenum filter);

  RemotableResource(RemotableResource&& other);

  ~RemotableResource();

  RemotableResource& operator=(RemotableResource&& other);

  bool needs_sync_token() const {
    return type != RemotableResourceType::kBitmap &&
           synchronization_state_ == LOCALLY_USED;
  }

  SynchronizationState synchronization_state() const {
    return synchronization_state_;
  }

  const TextureMailbox& mailbox() const { return mailbox_; }
  void SetMailbox(const TextureMailbox& mailbox);

  void SetLocallyUsed();
  void SetSynchronized();
  void UpdateSyncToken(const gpu::SyncToken& sync_token);
  int8_t* GetSyncTokenData();
  // If true the texture-backed or GpuMemoryBuffer-backed resource needs its
  // SyncToken waited on in order to be synchronized for use.
  bool ShouldWaitSyncToken() const;
  void SetGenerateMipmap();

  // In the service, this is the id of the client the resource comes from.
  int child_id;
  // In the service, this is the id of the resource in the client's namespace.
  ResourceId id_in_child;

  // Texture id for texture-backed resources.
  GLuint gl_id;
  // Callback to run when the resource is deleted and not in use by the service.
  ReleaseCallback release_callback;
  // Non-owning pointer to a software-backed resource when mapped.
  uint8_t* pixels;

  // Reference-counts to know when a resource can be released through the
  // |release_callback| after it is deleted, and for verifying correct use
  // of the resource.
  int lock_for_read_count;
  int imported_count;
  int exported_count;

  bool locked_for_write : 1;
  // When true the resource can not be used and must only be deleted. This is
  // passed along to the |release_callback|.
  bool lost : 1;
  // When the resource should be deleted until it is actually reaped.
  bool marked_for_deletion : 1;
  // When false, the resource backing hasn't been allocated yet.
  bool allocated : 1;
  // Tracks if a gpu fence needs to be used for reading a GpuMemoryBuffer-
  // backed or texture-backed resource.
  bool read_lock_fences_enabled : 1;
  // True if the software-backed resource is in shared memory, in which case
  // |shared_bitmap_id| will be valid.
  bool has_shared_bitmap_id : 1;
  // When true, the resource should be considered for being displayed in an
  // overlay.
  bool is_overlay_candidate : 1;

#if defined(OS_ANDROID)
  // Indicates whether this resource may not be overlayed on Android, since
  // it's not backed by a SurfaceView.  This may be set in combination with
  // |is_overlay_candidate|, to find out if switching the resource to a
  // a SurfaceView would result in overlay promotion.  It's good to find this
  // out in advance, since one has no fallback path for displaying a
  // SurfaceView except via promoting it to an overlay.  Ideally, one _could_
  // promote SurfaceTexture via the overlay path, even if one ended up just
  // drawing a quad in the compositor.  However, for now, we use this flag to
  // refuse to promote so that the compositor will draw the quad.
  bool is_backed_by_surface_texture : 1;
  // Indicates that this resource would like a promotion hint.
  bool wants_promotion_hint : 1;
#endif

  // A fence used for accessing a GpuMemoryBuffer-backed or texture-backed
  // resource for reading, that ensures any writing done to the resource has
  // been completed. This is implemented and used to implement transferring
  // ownership of the resource from the client to the service, and in the GL
  // drawing code before reading from the texture.
  scoped_refptr<RemotableResourceFence> read_lock_fence;

  // Size of the resource in pixels.
  gfx::Size size;
  // Where the resource was originally allocated. Either internally by the
  // ResourceProvider instance, externally and given to the ResourceProvider
  // via in-process methods, or in a client and given to the ResourceProvider
  // via IPC.
  Origin origin;
  // The texture target for GpuMemoryBuffer- and texture-backed resources.
  GLenum target;
  // The min/mag filter of the resource when it was given to/created by the
  // ResourceProvider, for texture-backed resources. Used to restore
  // the filter before releasing the resource. Not used for GpuMemoryBuffer-
  // backed resources as they are always internally created, so not released.
  // TODO(skyostil): Use a separate sampler object for filter state.
  GLenum original_filter;
  // The current mag filter for GpuMemoryBuffer- and texture-backed resources.
  GLenum filter;
  // The current min filter for GpuMemoryBuffer- and texture-backed resources.
  GLenum min_filter;
  // The GL image id for GpuMemoryBuffer-backed resources.
  GLuint image_id;

  // A hint for texture-backed resources about how the resource will be used,
  // that dictates how it should be allocated/used.
  RemotableResourceTextureHint hint;
  // The type of backing for the resource (such as gpu vs software).
  RemotableResourceType type;
  // GpuMemoryBuffer resource allocation needs to know how the resource will
  // be used.
  gfx::BufferUsage usage;
  // This is the the actual format of the underlaying GpuMemoryBuffer, if any,
  // and might not correspond to ResourceFormat. This format is needed to
  // scanout the buffer as HW overlay.
  gfx::BufferFormat buffer_format;
  // The format as seen from the compositor and might not correspond to
  // buffer_format (e.g: A resouce that was created from a YUV buffer could be
  // seen as RGB from the compositor/GL).
  ResourceFormat format;
  // The name of the shared memory for software-backed resources, but not
  // present if the resource isn't shared memory.
  SharedBitmapId shared_bitmap_id;
  // A pointer to the shared memory structure for software-backed resources, but
  // not present if the resources isn't shared memory. May be an owning pointer
  // or a weak pointer.
  SharedBitmap* shared_bitmap;
  // The GpuMemoryBuffer ownership for GpuMemoryBuffer-backed resources.
  std::unique_ptr<gfx::GpuMemoryBuffer> gpu_memory_buffer;
  // The color space for all resource types, to control how the resource should
  // be drawn to output device.
  gfx::ColorSpace color_space;
  // Used to track generating mipmaps for texture-backed resources.
  MipmapState mipmap_state = INVALID;

 private:
  SynchronizationState synchronization_state_ = SYNCHRONIZED;
  TextureMailbox mailbox_;

  DISALLOW_COPY_AND_ASSIGN(RemotableResource);
};

}  // namespace internal
}  // namespace viz

#endif  // COMPONENTS_VIZ_COMMON_RESOURCES_REMOTABLE_RESOURCE_H_
