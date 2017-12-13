// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_CLIENT_LOCAL_SURFACE_ID_PROVIDER_H_
#define COMPONENTS_VIZ_CLIENT_LOCAL_SURFACE_ID_PROVIDER_H_

#include "components/viz/client/viz_client_export.h"
#include "components/viz/common/surfaces/local_surface_id.h"
#include "components/viz/common/surfaces/parent_local_surface_id_allocator.h"
#include "ui/gfx/geometry/size.h"

namespace viz {
class CompositorFrame;

class VIZ_CLIENT_EXPORT LocalSurfaceIdProvider {
 public:
  LocalSurfaceIdProvider();
  virtual ~LocalSurfaceIdProvider();

  virtual const LocalSurfaceId& GetLocalSurfaceIdForFrame(
      const CompositorFrame& frame) = 0;

  virtual void SetNeedsNewSurface() = 0;

 protected:
  DISALLOW_COPY_AND_ASSIGN(LocalSurfaceIdProvider);
};

class VIZ_CLIENT_EXPORT DefaultLocalSurfaceIdProvider
    : public LocalSurfaceIdProvider {
 public:
  DefaultLocalSurfaceIdProvider();

  // LocalSurfaceIdProvider implementation.
  const LocalSurfaceId& GetLocalSurfaceIdForFrame(
      const CompositorFrame& frame) override;
  void SetNeedsNewSurface() override;

 private:
  LocalSurfaceId local_surface_id_;
  gfx::Size surface_size_;
  float device_scale_factor_ = 0;
  bool needs_new_surface_ = false;
  ParentLocalSurfaceIdAllocator parent_local_surface_id_allocator_;

  DISALLOW_COPY_AND_ASSIGN(DefaultLocalSurfaceIdProvider);
};

}  //  namespace viz

#endif  // COMPONENTS_VIZ_CLIENT_LOCAL_SURFACE_ID_PROVIDER_H_
