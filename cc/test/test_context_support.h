// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_TEST_TEST_CONTEXT_SUPPORT_H_
#define CC_TEST_TEST_CONTEXT_SUPPORT_H_

#include <vector>

#include "base/memory/weak_ptr.h"
#include "gpu/command_buffer/client/context_support.h"

namespace cc {

class TestContextSupport : public gpu::ContextSupport {
 public:
  TestContextSupport();
  virtual ~TestContextSupport();

  // gpu::ContextSupport implementation.
  virtual void SignalSyncPoint(uint32 sync_point,
                               const base::Closure& callback) override;
  virtual void SignalQuery(uint32 query,
                           const base::Closure& callback) override;
  virtual void SetSurfaceVisible(bool visible) override;
  virtual void Swap() override;
  virtual void PartialSwapBuffers(const gfx::Rect& sub_buffer) override;
  virtual uint32 InsertFutureSyncPointCHROMIUM() override;
  virtual void RetireSyncPointCHROMIUM(uint32 sync_point) override;
  virtual void ScheduleOverlayPlane(int plane_z_order,
                                    gfx::OverlayTransform plane_transform,
                                    unsigned overlay_texture_id,
                                    const gfx::Rect& display_bounds,
                                    const gfx::RectF& uv_rect) override;

  void CallAllSyncPointCallbacks();

  typedef base::Callback<void(bool visible)> SurfaceVisibleCallback;
  void SetSurfaceVisibleCallback(
      const SurfaceVisibleCallback& set_visible_callback);

  typedef base::Callback<void(int plane_z_order,
                              gfx::OverlayTransform plane_transform,
                              unsigned overlay_texture_id,
                              const gfx::Rect& display_bounds,
                              const gfx::RectF& crop_rect)>
      ScheduleOverlayPlaneCallback;
  void SetScheduleOverlayPlaneCallback(
      const ScheduleOverlayPlaneCallback& schedule_overlay_plane_callback);

 private:
  std::vector<base::Closure> sync_point_callbacks_;
  SurfaceVisibleCallback set_visible_callback_;
  ScheduleOverlayPlaneCallback schedule_overlay_plane_callback_;

  base::WeakPtrFactory<TestContextSupport> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(TestContextSupport);
};

}  // namespace cc

#endif  // CC_TEST_TEST_CONTEXT_SUPPORT_H_
