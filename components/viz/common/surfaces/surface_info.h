// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_COMMON_SURFACES_SURFACE_INFO_H_
#define COMPONENTS_VIZ_COMMON_SURFACES_SURFACE_INFO_H_

#include "components/viz/common/surfaces/surface_id.h"
#include "ui/gfx/geometry/size.h"

namespace IPC {
template <class T>
struct ParamTraits;
}  // namespace IPC

namespace viz {

namespace mojom {
class SurfaceInfoDataView;
}  // namespace mojom

// This class contains information about the surface that is being embedded.
class SurfaceInfo {
 public:
  SurfaceInfo() = default;
  SurfaceInfo(const SurfaceId& id, const gfx::Size& size)
      : id_(id), size_(size) {}

  bool is_valid() const { return id_.is_valid() && !size_.IsEmpty(); }

  bool operator==(const SurfaceInfo& info) const {
    return id_ == info.id() && size_ == info.size();
  }

  bool operator!=(const SurfaceInfo& info) const { return !(*this == info); }

  const SurfaceId& id() const { return id_; }
  const gfx::Size& size() const { return size_; }

 private:
  friend struct mojo::StructTraits<mojom::SurfaceInfoDataView, SurfaceInfo>;
  friend struct IPC::ParamTraits<SurfaceInfo>;

  SurfaceId id_;
  gfx::Size size_;
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_COMMON_SURFACES_SURFACE_INFO_H_
