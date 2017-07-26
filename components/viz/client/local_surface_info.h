// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_CLIENT_LOCAL_SURFACE_INFO_H_
#define COMPONENTS_VIZ_CLIENT_LOCAL_SURFACE_INFO_H_

#include "components/viz/common/surfaces/local_surface_id.h"
#include "ui/gfx/geometry/size.h"

namespace viz {

// This class contains information about the surface that is being embedded.
class LocalSurfaceInfo {
 public:
  LocalSurfaceInfo() = default;
  LocalSurfaceInfo(const LocalSurfaceId& id,
                   float device_scale_factor,
                   const gfx::Size& size_in_pixels)
      : id_(id),
        device_scale_factor_(device_scale_factor),
        size_in_pixels_(size_in_pixels) {}

  bool is_valid() const {
    return id_.is_valid() && device_scale_factor_ != 0 &&
           !size_in_pixels_.IsEmpty();
  }

  bool operator==(const LocalSurfaceInfo& info) const {
    return id_ == info.id() &&
           device_scale_factor_ == info.device_scale_factor() &&
           size_in_pixels_ == info.size_in_pixels();
  }

  bool operator!=(const LocalSurfaceInfo& info) const {
    return !(*this == info);
  }

  const LocalSurfaceId& id() const { return id_; }
  float device_scale_factor() const { return device_scale_factor_; }
  const gfx::Size& size_in_pixels() const { return size_in_pixels_; }

 private:
  LocalSurfaceId id_;
  float device_scale_factor_ = 1.f;
  gfx::Size size_in_pixels_;
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_CLIENT_LOCAL_SURFACE_INFO_H_
