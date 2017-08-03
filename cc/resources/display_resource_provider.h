// Copyright 2017 The Chromium Authors. All rights reserved.
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

 private:
  DISALLOW_COPY_AND_ASSIGN(DisplayResourceProvider);
};

}  // namespace cc

#endif  // CC_RESOURCES_DISPLAY_RESOURCE_PROVIDER_H_
