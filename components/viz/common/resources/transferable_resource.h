// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_COMMON_RESOURCES_TRANSFERABLE_RESOURCE_H_
#define COMPONENTS_VIZ_COMMON_RESOURCES_TRANSFERABLE_RESOURCE_H_

#include <stdint.h>

#include <vector>

#include "build/build_config.h"
#include "components/viz/common/resources/resource_format.h"
#include "components/viz/common/resources/resource_id.h"
#include "components/viz/common/viz_common_export.h"
#include "gpu/command_buffer/common/mailbox_holder.h"
#include "ui/gfx/buffer_types.h"
#include "ui/gfx/color_space.h"
#include "ui/gfx/geometry/size.h"

namespace viz {

struct ReturnedResource;

struct VIZ_COMMON_EXPORT TransferableResource {
  TransferableResource();
  TransferableResource(const TransferableResource& other);
  ~TransferableResource();

  ReturnedResource ToReturnedResource() const;
  static std::vector<ReturnedResource> ReturnResources(
      const std::vector<TransferableResource>& input);

  ResourceId id;
  // Refer to ResourceProvider::Resource for the meaning of the following data.
  ResourceFormat format;
  gfx::BufferFormat buffer_format;
  uint32_t filter;
  gfx::Size size;
  gpu::MailboxHolder mailbox_holder;
  bool read_lock_fences_enabled;
  bool is_software;
  uint32_t shared_bitmap_sequence_number;
#if defined(OS_ANDROID)
  bool is_backed_by_surface_texture;
  bool wants_promotion_hint;
#endif
  bool is_overlay_candidate;
  gfx::ColorSpace color_space;

  bool Equals(const TransferableResource& o) const {
    return id == o.id && format == o.format &&
           buffer_format == o.buffer_format && filter == o.filter &&
           size == o.size &&
           mailbox_holder.mailbox == o.mailbox_holder.mailbox &&
           mailbox_holder.sync_token == o.mailbox_holder.sync_token &&
           mailbox_holder.texture_target == o.mailbox_holder.texture_target &&
           read_lock_fences_enabled == o.read_lock_fences_enabled &&
           is_software == o.is_software &&
           shared_bitmap_sequence_number == o.shared_bitmap_sequence_number &&
#if defined(OS_ANDROID)
           is_backed_by_surface_texture == o.is_backed_by_surface_texture &&
           wants_promotion_hint == o.wants_promotion_hint &&
#endif
           is_overlay_candidate == o.is_overlay_candidate &&
           color_space == o.color_space;
  }
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_COMMON_RESOURCES_TRANSFERABLE_RESOURCE_H_
