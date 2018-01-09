// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_VIZ_PUBLIC_CPP_COMPOSITING_RENDER_FRAME_METADATA_STRUCT_TRAITS_H_
#define SERVICES_VIZ_PUBLIC_CPP_COMPOSITING_RENDER_FRAME_METADATA_STRUCT_TRAITS_H_

#include "components/viz/common/quads/render_frame_metadata.h"
#include "services/viz/public/interfaces/compositing/render_frame_metadata.mojom-shared.h"

namespace mojo {

template <>
struct StructTraits<viz::mojom::RenderFrameMetadataDataView,
                    viz::RenderFrameMetadata> {
  static gfx::Vector2dF root_scroll_offset(
      const viz::RenderFrameMetadata& metadata) {
    return metadata.root_scroll_offset;
  }

  static bool Read(viz::mojom::RenderFrameMetadataDataView data,
                   viz::RenderFrameMetadata* out);
};

}  // namespace mojo

#endif  // SERVICES_VIZ_PUBLIC_CPP_COMPOSITING_RENDER_FRAME_METADATA_STRUCT_TRAITS_H_
