// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/resources/layer_tree_resource_provider.h"

#include "gpu/command_buffer/client/gles2_interface.h"

using gpu::gles2::GLES2Interface;

namespace cc {

LayerTreeResourceProvider::LayerTreeResourceProvider(
    viz::ContextProvider* compositor_context_provider,
    viz::SharedBitmapManager* shared_bitmap_manager,
    gpu::GpuMemoryBufferManager* gpu_memory_buffer_manager,
    BlockingTaskRunner* blocking_main_thread_task_runner,
    bool delegated_sync_points_required,
    bool enable_color_correct_rasterization,
    const viz::ResourceSettings& resource_settings)
    : ResourceProvider(compositor_context_provider,
                       shared_bitmap_manager,
                       gpu_memory_buffer_manager,
                       blocking_main_thread_task_runner,
                       delegated_sync_points_required,
                       enable_color_correct_rasterization,
                       resource_settings) {}

LayerTreeResourceProvider::~LayerTreeResourceProvider() {}

void LayerTreeResourceProvider::PrepareSendToParent(
    const ResourceIdArray& resource_ids,
    std::vector<viz::TransferableResource>* list) {
  DCHECK(thread_checker_.CalledOnValidThread());
  GLES2Interface* gl = ContextGL();

  // This function goes through the array multiple times, store the resources
  // as pointers so we don't have to look up the resource id multiple times.
  std::vector<Resource*> resources;
  resources.reserve(resource_ids.size());
  for (const viz::ResourceId id : resource_ids)
    resources.push_back(GetResource(id));

  // Lazily create any mailboxes and verify all unverified sync tokens.
  std::vector<GLbyte*> unverified_sync_tokens;
  std::vector<Resource*> need_synchronization_resources;
  for (Resource* resource : resources) {
    if (!ResourceProvider::IsGpuResourceType(resource->type))
      continue;

    CreateMailboxAndBindResource(gl, resource);

    if (settings_.delegated_sync_points_required) {
      if (resource->needs_sync_token()) {
        need_synchronization_resources.push_back(resource);
      } else if (!resource->mailbox().sync_token().verified_flush()) {
        unverified_sync_tokens.push_back(resource->GetSyncTokenData());
      }
    }
  }

  // Insert sync point to synchronize the mailbox creation or bound textures.
  gpu::SyncToken new_sync_token;
  if (!need_synchronization_resources.empty()) {
    DCHECK(settings_.delegated_sync_points_required);
    DCHECK(gl);
    const uint64_t fence_sync = gl->InsertFenceSyncCHROMIUM();
    gl->OrderingBarrierCHROMIUM();
    gl->GenUnverifiedSyncTokenCHROMIUM(fence_sync, new_sync_token.GetData());
    unverified_sync_tokens.push_back(new_sync_token.GetData());
  }

  if (!unverified_sync_tokens.empty()) {
    DCHECK(settings_.delegated_sync_points_required);
    DCHECK(gl);
    gl->VerifySyncTokensCHROMIUM(unverified_sync_tokens.data(),
                                 unverified_sync_tokens.size());
  }

  // Set sync token after verification.
  for (Resource* resource : need_synchronization_resources) {
    DCHECK(ResourceProvider::IsGpuResourceType(resource->type));
    resource->UpdateSyncToken(new_sync_token);
    resource->SetSynchronized();
  }

  // Transfer Resources
  DCHECK_EQ(resources.size(), resource_ids.size());
  for (size_t i = 0; i < resources.size(); ++i) {
    Resource* source = resources[i];
    const viz::ResourceId id = resource_ids[i];

    DCHECK(!settings_.delegated_sync_points_required ||
           !source->needs_sync_token());
    DCHECK(!settings_.delegated_sync_points_required ||
           Resource::LOCALLY_USED != source->synchronization_state());

    viz::TransferableResource resource;
    TransferResource(source, id, &resource);

    source->exported_count++;
    list->push_back(resource);
  }
}

void LayerTreeResourceProvider::ReceiveReturnsFromParent(
    const std::vector<viz::ReturnedResource>& resources) {
  DCHECK(thread_checker_.CalledOnValidThread());
  GLES2Interface* gl = ContextGL();

  std::unordered_map<int, ResourceIdArray> resources_for_child;

  for (const viz::ReturnedResource& returned : resources) {
    viz::ResourceId local_id = returned.id;
    ResourceMap::iterator map_iterator = resources_.find(local_id);
    // Resource was already lost (e.g. it belonged to a child that was
    // destroyed).
    if (map_iterator == resources_.end())
      continue;

    Resource* resource = &map_iterator->second;

    CHECK_GE(resource->exported_count, returned.count);
    resource->exported_count -= returned.count;
    resource->lost |= returned.lost;
    if (resource->exported_count)
      continue;

    if (returned.sync_token.HasData()) {
      DCHECK(!resource->has_shared_bitmap_id);
      if (resource->origin == Resource::INTERNAL) {
        DCHECK(resource->gl_id);
        DCHECK(returned.sync_token.HasData());
        gl->WaitSyncTokenCHROMIUM(returned.sync_token.GetConstData());
        resource->SetSynchronized();
      } else {
        DCHECK(!resource->gl_id);
        resource->UpdateSyncToken(returned.sync_token);
      }
    }

    if (!resource->marked_for_deletion)
      continue;

    if (!resource->child_id) {
      // The resource belongs to this LayerTreeResourceProvider, so it can be
      // destroyed.
      DeleteResourceInternal(map_iterator, NORMAL);
      continue;
    }

    DCHECK(resource->origin == Resource::DELEGATED);
    resources_for_child[resource->child_id].push_back(local_id);
  }

  for (const auto& children : resources_for_child) {
    ChildMap::iterator child_it = children_.find(children.first);
    DCHECK(child_it != children_.end());
    DeleteAndReturnUnusedResourcesToChild(child_it, NORMAL, children.second);
  }
}

}  // namespace cc
