// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/resources/display_resource_provider.h"

#include "base/trace_event/trace_event.h"
#include "build/build_config.h"
#include "gpu/command_buffer/client/gles2_interface.h"
#include "third_party/skia/include/gpu/GrBackendSurface.h"
#include "third_party/skia/include/gpu/gl/GrGLTypes.h"

using gpu::gles2::GLES2Interface;

namespace cc {

GrPixelConfig ToGrPixelConfig(viz::ResourceFormat format) {
  switch (format) {
    case viz::RGBA_8888:
      return kRGBA_8888_GrPixelConfig;
    case viz::BGRA_8888:
      return kBGRA_8888_GrPixelConfig;
    case viz::RGBA_4444:
      return kRGBA_4444_GrPixelConfig;
    case viz::RGBA_F16:
      return kRGBA_half_GrPixelConfig;
    default:
      break;
  }
  DCHECK(false) << "Unsupported resource format.";
  return kSkia8888_GrPixelConfig;
}

DisplayResourceProvider::DisplayResourceProvider(
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

DisplayResourceProvider::~DisplayResourceProvider() {}

int DisplayResourceProvider::CreateChild(
    const ReturnCallback& return_callback) {
  DCHECK(thread_checker_.CalledOnValidThread());

  Child child_info;
  child_info.return_callback = return_callback;

  int child = next_child_++;
  children_[child] = child_info;
  return child;
}

void DisplayResourceProvider::SetChildNeedsSyncTokens(int child_id,
                                                      bool needs) {
  ChildMap::iterator it = children_.find(child_id);
  DCHECK(it != children_.end());
  it->second.needs_sync_tokens = needs;
}

void DisplayResourceProvider::DestroyChild(int child_id) {
  ChildMap::iterator it = children_.find(child_id);
  DCHECK(it != children_.end());
  DestroyChildInternal(it, NORMAL);
}

void DisplayResourceProvider::ReceiveFromChild(
    int child,
    const std::vector<viz::TransferableResource>& resources) {
  DCHECK(thread_checker_.CalledOnValidThread());
  GLES2Interface* gl = ContextGL();
  Child& child_info = children_.find(child)->second;
  DCHECK(!child_info.marked_for_deletion);
  for (std::vector<viz::TransferableResource>::const_iterator it =
           resources.begin();
       it != resources.end(); ++it) {
    ResourceIdMap::iterator resource_in_map_it =
        child_info.child_to_parent_map.find(it->id);
    if (resource_in_map_it != child_info.child_to_parent_map.end()) {
      Resource* resource = GetResource(resource_in_map_it->second);
      resource->marked_for_deletion = false;
      resource->imported_count++;
      continue;
    }

    if ((!it->is_software && !gl) ||
        (it->is_software && !shared_bitmap_manager_)) {
      TRACE_EVENT0("cc", "ResourceProvider::ReceiveFromChild dropping invalid");
      std::vector<viz::ReturnedResource> to_return;
      to_return.push_back(it->ToReturnedResource());
      child_info.return_callback.Run(to_return,
                                     blocking_main_thread_task_runner_);
      continue;
    }

    viz::ResourceId local_id = next_id_++;
    Resource* resource = nullptr;
    if (it->is_software) {
      resource = InsertResource(local_id,
                                Resource(it->mailbox_holder.mailbox, it->size,
                                         Resource::DELEGATED, GL_LINEAR));
    } else {
      resource = InsertResource(
          local_id, Resource(0, it->size, Resource::DELEGATED,
                             it->mailbox_holder.texture_target, it->filter,
                             TEXTURE_HINT_IMMUTABLE, RESOURCE_TYPE_GL_TEXTURE,
                             it->format));
      resource->buffer_format = it->buffer_format;
      resource->SetMailbox(viz::TextureMailbox(
          it->mailbox_holder.mailbox, it->mailbox_holder.sync_token,
          it->mailbox_holder.texture_target));
      resource->read_lock_fences_enabled = it->read_lock_fences_enabled;
      resource->is_overlay_candidate = it->is_overlay_candidate;
#if defined(OS_ANDROID)
      resource->is_backed_by_surface_texture = it->is_backed_by_surface_texture;
      resource->wants_promotion_hint = it->wants_promotion_hint;
      if (resource->wants_promotion_hint)
        wants_promotion_hints_set_.insert(local_id);
#endif
      resource->color_space = it->color_space;
    }
    resource->child_id = child;
    // Don't allocate a texture for a child.
    resource->allocated = true;
    resource->imported_count = 1;
    resource->id_in_child = it->id;
    child_info.child_to_parent_map[it->id] = local_id;
  }
}

void DisplayResourceProvider::DeclareUsedResourcesFromChild(
    int child,
    const viz::ResourceIdSet& resources_from_child) {
  DCHECK(thread_checker_.CalledOnValidThread());

  ChildMap::iterator child_it = children_.find(child);
  DCHECK(child_it != children_.end());
  Child& child_info = child_it->second;
  DCHECK(!child_info.marked_for_deletion);

  ResourceIdArray unused;
  for (ResourceIdMap::iterator it = child_info.child_to_parent_map.begin();
       it != child_info.child_to_parent_map.end(); ++it) {
    viz::ResourceId local_id = it->second;
    bool resource_is_in_use = resources_from_child.count(it->first) > 0;
    if (!resource_is_in_use)
      unused.push_back(local_id);
  }
  DeleteAndReturnUnusedResourcesToChild(child_it, NORMAL, unused);
}

const ResourceProvider::ResourceIdMap&
DisplayResourceProvider::GetChildToParentMap(int child) const {
  DCHECK(thread_checker_.CalledOnValidThread());
  ChildMap::const_iterator it = children_.find(child);
  DCHECK(it != children_.end());
  DCHECK(!it->second.marked_for_deletion);
  return it->second.child_to_parent_map;
}

}  // namespace cc
