// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_DRM_HOST_DRM_OVERLAY_CANDIDATES_CORE_H_
#define UI_OZONE_PLATFORM_DRM_HOST_DRM_OVERLAY_CANDIDATES_CORE_H_

#include <stdint.h>

#include <deque>
#include <map>
#include <vector>

#include "base/containers/mru_cache.h"
#include "base/macros.h"
#include "base/memory/scoped_vector.h"
#include "ui/ozone/common/gpu/ozone_gpu_message_params.h"
#include "ui/ozone/public/overlay_candidates_ozone.h"

namespace ui {

class DrmWindowHost;

// Pluggable sender proxy abstract base class.
class DrmOverlayCandidatesHostProxy {
 public:
  virtual ~DrmOverlayCandidatesHostProxy() = 0;
  virtual bool IsConnected() = 0;
  virtual bool CheckOverlayCapabilities(
      gfx::AcceleratedWidget widget,
      const std::vector<OverlayCheck_Params>& new_params) = 0;
  virtual void UnregisterHandler() = 0;
  virtual void RegisterHandler() = 0;
};

// This is the core functionality of DrmOverlayCandidatesHost where the
// communication with the GPU thread is via a pluggable proxy.
class DrmOverlayCandidatesHostCore : public OverlayCandidatesOzone {
 public:
  DrmOverlayCandidatesHostCore(DrmOverlayCandidatesHostProxy* proxy,
                               DrmWindowHost* window);
  ~DrmOverlayCandidatesHostCore() override;

  // OverlayCandidatesOzone:
  void CheckOverlaySupport(OverlaySurfaceCandidateList* candidates) override;

  void ResetCache();
  void GpuSentOverlayResult(bool* handled,
                            gfx::AcceleratedWidget widget,
                            const std::vector<OverlayCheck_Params>& params);

 private:
  void SendOverlayValidationRequest(
      const std::vector<OverlayCheck_Params>& list) const;
  bool CanHandleCandidate(const OverlaySurfaceCandidate& candidate) const;

  DrmOverlayCandidatesHostProxy* proxy_;  // Not owned.
  DrmWindowHost* window_;                 // Not owned.

  // List of all OverlayCheck_Params which have been validated in GPU side.
  // Value is set to true if we are waiting for validation results from GPU.
  base::MRUCacheBase<std::vector<OverlayCheck_Params>,
                     bool,
                     base::MRUCacheNullDeletor<bool>> cache_;

  DISALLOW_COPY_AND_ASSIGN(DrmOverlayCandidatesHostCore);
};

}  // namespace ui

#endif  // UI_OZONE_PLATFORM_DRM_HOST_DRM_OVERLAY_CANDIDATES_CORE_H_
