// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_RESOURCES_DISPLAY_RESOURCE_PROVIDER_H_
#define CC_RESOURCES_DISPLAY_RESOURCE_PROVIDER_H_

#include "cc/resources/resource_provider.h"

namespace viz {
class SharedBitmapManager;
}  // namespace viz

namespace cc {
class BlockingTaskRunner;

// This class is not thread-safe and can only be called from the thread it was
// created on (in practice, the impl thread).
class CC_EXPORT DisplayResourceProvider : public ResourceProvider {
 public:
  DisplayResourceProvider(
      viz::ContextProvider* compositor_context_provider,
      viz::SharedBitmapManager* shared_bitmap_manager,
      gpu::GpuMemoryBufferManager* gpu_memory_buffer_manager,
      BlockingTaskRunner* blocking_main_thread_task_runner,
      bool delegated_sync_points_required,
      bool enable_color_correct_rasterization,
      const viz::ResourceSettings& resource_settings);
  ~DisplayResourceProvider() override;

  // Creates accounting for a child. Returns a child ID.
  int CreateChild(const ReturnCallback& return_callback);

  // Destroys accounting for the child, deleting all accounted resources.
  void DestroyChild(int child);

  // Sets whether resources need sync points set on them when returned to this
  // child. Defaults to true.
  void SetChildNeedsSyncTokens(int child, bool needs_sync_tokens);

  // Gets the child->parent resource ID map.
  const ResourceIdMap& GetChildToParentMap(int child) const;

  // Receives resources from a child, moving them from mailboxes. Resource IDs
  // passed are in the child namespace, and will be translated to the parent
  // namespace, added to the child->parent map.
  // This adds the resources to the working set in the ResourceProvider without
  // declaring which resources are in use. Use DeclareUsedResourcesFromChild
  // after calling this method to do that. All calls to ReceiveFromChild should
  // be followed by a DeclareUsedResourcesFromChild.
  // NOTE: if the sync_token is set on any viz::TransferableResource, this will
  // wait on it.
  void ReceiveFromChild(
      int child,
      const std::vector<viz::TransferableResource>& transferable_resources);

  // Once a set of resources have been received, they may or may not be used.
  // This declares what set of resources are currently in use from the child,
  // releasing any other resources back to the child.
  void DeclareUsedResourcesFromChild(
      int child,
      const viz::ResourceIdSet& resources_from_child);

  // The following lock classes are part of the ResourceProvider API and are
  // needed to read and write the resource contents. The user must ensure
  // that they only use GL locks on GL resources, etc, and this is enforced
  // by assertions.
  class CC_EXPORT ScopedReadLockGL {
   public:
    ScopedReadLockGL(DisplayResourceProvider* resource_provider,
                     viz::ResourceId resource_id);
    ~ScopedReadLockGL();

    unsigned texture_id() const { return texture_id_; }
    GLenum target() const { return target_; }
    const gfx::Size& size() const { return size_; }
    const gfx::ColorSpace& color_space() const { return color_space_; }

   private:
    DisplayResourceProvider* resource_provider_;
    viz::ResourceId resource_id_;
    unsigned texture_id_;
    GLenum target_;
    gfx::Size size_;
    gfx::ColorSpace color_space_;

    DISALLOW_COPY_AND_ASSIGN(ScopedReadLockGL);
  };

  class CC_EXPORT ScopedSamplerGL {
   public:
    ScopedSamplerGL(DisplayResourceProvider* resource_provider,
                    viz::ResourceId resource_id,
                    GLenum filter);
    ScopedSamplerGL(DisplayResourceProvider* resource_provider,
                    viz::ResourceId resource_id,
                    GLenum unit,
                    GLenum filter);
    ~ScopedSamplerGL();

    unsigned texture_id() const { return resource_lock_.texture_id(); }
    GLenum target() const { return target_; }
    const gfx::ColorSpace& color_space() const {
      return resource_lock_.color_space();
    }

   private:
    ScopedReadLockGL resource_lock_;
    GLenum unit_;
    GLenum target_;

    DISALLOW_COPY_AND_ASSIGN(ScopedSamplerGL);
  };

  class CC_EXPORT ScopedReadLockSoftware {
   public:
    ScopedReadLockSoftware(DisplayResourceProvider* resource_provider,
                           viz::ResourceId resource_id);
    ~ScopedReadLockSoftware();

    const SkBitmap* sk_bitmap() const {
      DCHECK(valid());
      return &sk_bitmap_;
    }

    bool valid() const { return !!sk_bitmap_.getPixels(); }

   private:
    DisplayResourceProvider* resource_provider_;
    viz::ResourceId resource_id_;
    SkBitmap sk_bitmap_;

    DISALLOW_COPY_AND_ASSIGN(ScopedReadLockSoftware);
  };

  class CC_EXPORT ScopedReadLockSkImage {
   public:
    ScopedReadLockSkImage(DisplayResourceProvider* resource_provider,
                          viz::ResourceId resource_id);
    ~ScopedReadLockSkImage();

    const SkImage* sk_image() const { return sk_image_.get(); }

    bool valid() const { return !!sk_image_; }

   private:
    DisplayResourceProvider* resource_provider_;
    viz::ResourceId resource_id_;
    sk_sp<SkImage> sk_image_;

    DISALLOW_COPY_AND_ASSIGN(ScopedReadLockSkImage);
  };

 private:
  DISALLOW_COPY_AND_ASSIGN(DisplayResourceProvider);
};

}  // namespace cc

#endif  // CC_RESOURCES_DISPLAY_RESOURCE_PROVIDER_H_
