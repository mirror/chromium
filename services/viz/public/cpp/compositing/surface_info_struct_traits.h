// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_VIZ_PUBLIC_CPP_COMPOSITING_SURFACE_INFO_STRUCT_TRAITS_H_
#define SERVICES_VIZ_PUBLIC_CPP_COMPOSITING_SURFACE_INFO_STRUCT_TRAITS_H_

#include "components/viz/common/surfaces/surface_info.h"
#include "services/viz/public/interfaces/compositing/surface_info.mojom-shared.h"

namespace mojo {

template <>
struct StructTraits<viz::mojom::SurfaceInfoDataView, viz::SurfaceInfo> {
  static const viz::SurfaceId& surface_id(
      const viz::SurfaceInfo& surface_info) {
    return surface_info.id();
  }

  static const gfx::Size& size(const viz::SurfaceInfo& surface_info) {
    return surface_info.size();
  }

  static bool Read(viz::mojom::SurfaceInfoDataView data,
                   viz::SurfaceInfo* out) {
    return data.ReadSurfaceId(&out->id_) && data.ReadSize(&out->size_) &&
           out->is_valid();
  }
};

}  // namespace mojo

#endif  // SERVICES_VIZ_PUBLIC_CPP_COMPOSITING_SURFACE_INFO_STRUCT_TRAITS_H_
