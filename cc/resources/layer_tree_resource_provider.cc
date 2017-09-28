// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/resources/layer_tree_resource_provider.h"

#include "build/build_config.h"
#include "components/viz/common/resources/resource_format_utils.h"
#include "gpu/command_buffer/client/context_support.h"
#include "gpu/command_buffer/client/gles2_interface.h"
#include "gpu/command_buffer/client/gpu_memory_buffer_manager.h"

using gpu::gles2::GLES2Interface;

namespace cc {

struct LayerTreeResourceProvider::ExportableTransferableResource {
  viz::ResourceId local_id;
  viz::TransferableResource transferable;
  std::unique_ptr<viz::SingleReleaseCallback> release_callback;
  int exported_count = 0;
  bool marked_for_deletion = false;

  gpu::SyncToken returned_sync_token;
  bool returned_lost = false;

  ExportableTransferableResource(
      viz::ResourceId local_id,
      const viz::TransferableResource& transferable,
      std::unique_ptr<viz::SingleReleaseCallback> release_callback)
      : local_id(local_id),
        transferable(transferable),
        release_callback(std::move(release_callback)) {}
  ~ExportableTransferableResource() = default;

  ExportableTransferableResource(ExportableTransferableResource&&) = default;
  ExportableTransferableResource& operator=(ExportableTransferableResource&&) =
      default;
};

LayerTreeResourceProvider::LayerTreeResourceProvider(
    viz::ContextProvider* compositor_context_provider,
    viz::SharedBitmapManager* shared_bitmap_manager,
    gpu::GpuMemoryBufferManager* gpu_memory_buffer_manager,
    bool delegated_sync_points_required,
    bool enable_color_correct_rasterization,
    const viz::ResourceSettings& resource_settings)
    : ResourceProvider(compositor_context_provider,
                       shared_bitmap_manager,
                       gpu_memory_buffer_manager,
                       delegated_sync_points_required,
                       enable_color_correct_rasterization,
                       resource_settings) {}

LayerTreeResourceProvider::~LayerTreeResourceProvider() {
  for (auto& pair : transferable_resources_) {
    ExportableTransferableResource& exportable = pair.second;
    exportable.release_callback->Run(gpu::SyncToken(), true /* is_lost */);
  }
}

gpu::SyncToken LayerTreeResourceProvider::GetSyncTokenForResources(
    const ResourceIdArray& resource_ids) {
  gpu::SyncToken latest_sync_token;
  for (viz::ResourceId id : resource_ids) {
    const gpu::SyncToken& sync_token = GetResource(id)->mailbox().sync_token();
    if (sync_token.release_count() > latest_sync_token.release_count())
      latest_sync_token = sync_token;
  }
  return latest_sync_token;
}

void LayerTreeResourceProvider::PrepareSendToParent(
    const ResourceIdArray& export_ids,
    std::vector<viz::TransferableResource>* list) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  GLES2Interface* gl = ContextGL();

  // This function goes through the array multiple times, store the resources
  // as pointers so we don't have to look up the resource id multiple times.
  // Make sure the maps do not change while these vectors are alive or they
  // will become invalid.
  std::vector<std::pair<Resource*, viz::ResourceId>> resources;
  std::vector<std::pair<ExportableTransferableResource*, viz::ResourceId>>
      exportables;
  resources.reserve(export_ids.size());
  exportables.reserve(export_ids.size());
  for (const viz::ResourceId id : export_ids) {
    auto it = transferable_resources_.find(id);
    if (it != transferable_resources_.end())
      exportables.push_back({&it->second, id});
    else
      resources.push_back({GetResource(id), id});
  }
  DCHECK_EQ(resources.size() + exportables.size(), export_ids.size());

  // Lazily create any mailboxes and verify all unverified sync tokens.
  std::vector<GLbyte*> unverified_sync_tokens;
  std::vector<Resource*> need_synchronization_resources;
  for (auto& pair : resources) {
    Resource* resource = pair.first;
    if (!ResourceProvider::IsGpuResourceType(resource->type))
      continue;

    CreateMailbox(resource);

    if (settings_.delegated_sync_points_required) {
      if (resource->needs_sync_token()) {
        need_synchronization_resources.push_back(resource);
      } else if (!resource->mailbox().sync_token().verified_flush()) {
        unverified_sync_tokens.push_back(resource->GetSyncTokenData());
      }
    }
  }
  for (auto& pair : exportables) {
    ExportableTransferableResource* exportable = pair.first;
    if (!exportable->transferable.is_software &&
        !exportable->transferable.mailbox_holder.sync_token.verified_flush()) {
      unverified_sync_tokens.push_back(
          exportable->transferable.mailbox_holder.sync_token.GetData());
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

  if (compositor_context_provider_)
    compositor_context_provider_->ContextSupport()->FlushPendingWork();

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

  // Transfer Resources.
  for (size_t i = 0; i < resources.size(); ++i) {
    Resource* source = resources[i].first;
    const viz::ResourceId id = resources[i].second;

    DCHECK(!settings_.delegated_sync_points_required ||
           !source->needs_sync_token());
    DCHECK(!settings_.delegated_sync_points_required ||
           Resource::LOCALLY_USED != source->synchronization_state());

    viz::TransferableResource resource;
    TransferResource(source, id, &resource);

    source->exported_count++;
    list->push_back(std::move(resource));
  }
  for (auto& pair : exportables) {
    ExportableTransferableResource* exportable = pair.first;
    const viz::ResourceId& id = pair.second;

    viz::TransferableResource resource = exportable->transferable;
    resource.id = id;

    exportable->exported_count++;
    list->push_back(std::move(resource));
  }
}

void LayerTreeResourceProvider::ReceiveReturnsFromParent(
    const std::vector<viz::ReturnedResource>& resources) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  GLES2Interface* gl = ContextGL();

  for (const viz::ReturnedResource& returned : resources) {
    viz::ResourceId local_id = returned.id;

    auto transferable_it = transferable_resources_.find(local_id);
    if (transferable_it != transferable_resources_.end()) {
      ExportableTransferableResource& exportable = transferable_it->second;

      CHECK_GE(exportable.exported_count, returned.count);
      exportable.exported_count -= returned.count;
      exportable.returned_lost |= returned.lost;

      if (exportable.exported_count)
        continue;

      if (returned.sync_token.HasData()) {
        DCHECK(!exportable.transferable.is_software);
        exportable.returned_sync_token = returned.sync_token;
      }

      if (exportable.marked_for_deletion) {
        exportable.release_callback->Run(exportable.returned_sync_token,
                                         exportable.returned_lost);
        transferable_resources_.erase(transferable_it);
      }

      continue;
    }

    auto map_iterator = resources_.find(local_id);
    DCHECK(map_iterator != resources_.end());
    // Resource was already lost (e.g. it belonged to a child that was
    // destroyed).
    // TODO(danakj): Remove this. There is no "child" here anymore, and
    // lost resources are still in the map until exported_count == 0.
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
        gl->WaitSyncTokenCHROMIUM(returned.sync_token.GetConstData());
        resource->SetSynchronized();
      } else {
        DCHECK(!resource->gl_id);
        resource->UpdateSyncToken(returned.sync_token);
      }
    }

    if (!resource->marked_for_deletion)
      continue;

    // The resource belongs to this LayerTreeResourceProvider, so it can be
    // destroyed.
    DeleteResourceInternal(map_iterator, NORMAL);
  }
}

viz::ResourceId LayerTreeResourceProvider::AddTransferrableResource(
    const viz::TransferableResource& transferable,
    std::unique_ptr<viz::SingleReleaseCallback> release_callback) {
  // This field is only for LayerTreeResourceProvider-created resources.
  DCHECK(!transferable.read_lock_fences_enabled);

  viz::ResourceId id = next_id_++;
  auto result = transferable_resources_.emplace(
      id, ExportableTransferableResource(id, transferable,
                                         std::move(release_callback)));
  DCHECK(result.second);  // If false, the id was already in the map.
  return id;
}

void LayerTreeResourceProvider::RemoveTransferrableResource(
    viz::ResourceId id) {
  auto it = transferable_resources_.find(id);
  DCHECK(it != transferable_resources_.end());
  ExportableTransferableResource& exportable = it->second;
  exportable.marked_for_deletion = true;
  if (exportable.exported_count == 0) {
    exportable.release_callback->Run(exportable.returned_sync_token,
                                     exportable.returned_lost);
    transferable_resources_.erase(it);
  }
}

void LayerTreeResourceProvider::TransferResource(
    Resource* source,
    viz::ResourceId id,
    viz::TransferableResource* resource) {
  DCHECK(!source->locked_for_write);
  DCHECK(!source->lock_for_read_count);
  DCHECK(source->origin != Resource::EXTERNAL || source->mailbox().IsValid());
  DCHECK(source->allocated);
  resource->id = id;
  resource->format = source->format;
  resource->buffer_format = source->buffer_format;
  resource->mailbox_holder.texture_target = source->target;
  resource->filter = source->filter;
  resource->size = source->size;
  resource->read_lock_fences_enabled = source->read_lock_fences_enabled;
  resource->is_overlay_candidate = source->is_overlay_candidate;
#if defined(OS_ANDROID)
  resource->is_backed_by_surface_texture = source->is_backed_by_surface_texture;
  resource->wants_promotion_hint = source->wants_promotion_hint;
#endif
  resource->color_space = source->color_space;

  if (source->type == RESOURCE_TYPE_BITMAP) {
    resource->mailbox_holder.mailbox = source->shared_bitmap_id;
    resource->is_software = true;
    if (source->shared_bitmap) {
      resource->shared_bitmap_sequence_number =
          source->shared_bitmap->sequence_number();
    } else {
      resource->shared_bitmap_sequence_number = 0;
    }
  } else {
    DCHECK(source->mailbox().IsValid());
    DCHECK(source->mailbox().IsTexture());
    // This is either an external resource, or a compositor resource that we
    // already exported. Make sure to forward the sync point that we were given.
    resource->mailbox_holder.mailbox = source->mailbox().mailbox();
    resource->mailbox_holder.texture_target = source->mailbox().target();
    resource->mailbox_holder.sync_token = source->mailbox().sync_token();
  }
}

LayerTreeResourceProvider::ScopedWriteLockGpuMemoryBuffer ::
    ScopedWriteLockGpuMemoryBuffer(LayerTreeResourceProvider* resource_provider,
                                   viz::ResourceId resource_id)
    : resource_provider_(resource_provider), resource_id_(resource_id) {
  Resource* resource = resource_provider->LockForWrite(resource_id);
  DCHECK(IsGpuResourceType(resource->type));
  size_ = resource->size;
  format_ = resource->format;
  usage_ = resource->usage;
  color_space_ = resource_provider->GetResourceColorSpaceForRaster(resource);
  gpu_memory_buffer_ = std::move(resource->gpu_memory_buffer);
}

LayerTreeResourceProvider::ScopedWriteLockGpuMemoryBuffer::
    ~ScopedWriteLockGpuMemoryBuffer() {
  Resource* resource = resource_provider_->GetResource(resource_id_);
  // Avoid crashing in release builds if GpuMemoryBuffer allocation fails.
  // http://crbug.com/554541
  if (gpu_memory_buffer_) {
    resource->gpu_memory_buffer = std::move(gpu_memory_buffer_);
    resource->allocated = true;
    resource->is_overlay_candidate = true;
    resource->buffer_format = resource->gpu_memory_buffer->GetFormat();
    // GpuMemoryBuffer provides direct access to the memory used by the GPU.
    // Read lock fences are required to ensure that we're not trying to map a
    // buffer that is currently in-use by the GPU.
    resource->read_lock_fences_enabled = true;
    resource_provider_->CreateAndBindImage(resource);
  }
  resource_provider_->UnlockForWrite(resource);
}

gfx::GpuMemoryBuffer* LayerTreeResourceProvider::
    ScopedWriteLockGpuMemoryBuffer::GetGpuMemoryBuffer() {
  if (!gpu_memory_buffer_) {
    gpu_memory_buffer_ =
        resource_provider_->gpu_memory_buffer_manager()->CreateGpuMemoryBuffer(
            size_, BufferFormat(format_), usage_, gpu::kNullSurfaceHandle);
    // Avoid crashing in release builds if GpuMemoryBuffer allocation fails.
    // http://crbug.com/554541
    if (gpu_memory_buffer_ && color_space_.IsValid())
      gpu_memory_buffer_->SetColorSpaceForScanout(color_space_);
  }
  return gpu_memory_buffer_.get();
}

}  // namespace cc
