// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_DRM_HOST_DRM_OVERLAY_MANAGER_H_
#define UI_OZONE_PLATFORM_DRM_HOST_DRM_OVERLAY_MANAGER_H_

#include "base/memory/scoped_ptr.h"
#include "ui/ozone/public/overlay_manager_ozone.h"

namespace ui {

class DrmOverlayManager : public OverlayManagerOzone {
 public:
  DrmOverlayManager(bool allow_surfaceless);
  ~DrmOverlayManager() override;

  // OverlayManagerOzone:
  OverlayCandidatesOzone* GetOverlayCandidates(
      gfx::AcceleratedWidget w) override;
  bool CanShowPrimaryPlaneAsOverlay() override;

 private:
  bool allow_surfaceless_;

  scoped_ptr<OverlayCandidatesOzone> candidates_;

  DISALLOW_COPY_AND_ASSIGN(DrmOverlayManager);
};

}  // namespace ui

#endif  // UI_OZONE_PLATFORM_DRM_HOST_DRM_OVERLAY_MANAGER_H_
