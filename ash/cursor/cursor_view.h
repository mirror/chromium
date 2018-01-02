// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_CURSOR_CURSOR_VIEW_H_
#define ASH_CURSOR_CURSOR_VIEW_H_

#include "ash/fast_ink/fast_ink_view.h"
#include "base/sequence_checker.h"
#include "components/viz/common/frame_sinks/delay_based_time_source.h"
#include "ui/compositor/compositor_observer.h"
#include "ui/compositor/compositor_vsync_manager.h"
#include "ui/events/ozone/chromeos/cursor_controller.h"

namespace base {
class SingleThreadTaskRunner;
class Thread;
}  // namespace base

namespace ash {

// CursorView class can be used to display a cursor image with minimal
// latency/jank and optional motion blur.
class CursorView : public FastInkView,
                   public viz::DelayBasedTimeSourceClient,
                   public ui::CursorController::CursorObserver,
                   public ui::CompositorObserver,
                   public ui::CompositorVSyncManager::Observer {
 public:
  CursorView(aura::Window* container,
             const gfx::Point& initial_location,
             bool is_motion_blur_enabled);
  ~CursorView() override;

  void SetCursorLocation(const gfx::Point& new_location);
  void SetCursorImage(const gfx::ImageSkia& cursor_image,
                      const gfx::Size& cursor_size,
                      const gfx::Point& cursor_hotspot);

  // ui::CursorController::CursorObserver overrides:
  void OnCursorLocationChanged(const gfx::PointF& location) override;

  // ui::CompositorObserver overrides:
  void OnCompositingDidCommit(ui::Compositor* compositor) override {}
  void OnCompositingStarted(ui::Compositor* compositor,
                            base::TimeTicks start_time) override {}
  void OnCompositingEnded(ui::Compositor* compositor) override {}
  void OnCompositingLockStateChanged(ui::Compositor* compositor) override {}
  void OnCompositingChildResizing(ui::Compositor* compositor) override {}
  void OnCompositingShuttingDown(ui::Compositor* compositor) override;

  // ui::CompositorVSyncManager::Observer overrides:
  void OnUpdateVSyncParameters(base::TimeTicks timebase,
                               base::TimeDelta interval) override;

  // viz::DelayBasedTimeSourceClient overrides:
  void OnTimerTick() override;

 private:
  void StationaryOnPaintThread();
  gfx::Rect CalculateCursorRectOnPaintThread() const;
  void SetActiveOnPaintThread(bool active);
  void SetTimebaseAndIntervalOnPaintThread(base::TimeTicks timebase,
                                           base::TimeDelta interval);

  // Constants that can be used on any thread.
  const bool is_motion_blur_enabled_;
  const scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner_;
  gfx::Transform buffer_to_screen_transform_;

  base::Lock lock_;
  // Shared state protected by |lock_|.
  gfx::Point new_location_;
  gfx::Size new_cursor_size_;
  gfx::ImageSkia new_cursor_image_;
  gfx::Point new_cursor_hotspot_;
  bool location_from_cursor_controller_ = false;

  // Paint thread state.
  gfx::Point location_;
  gfx::ImageSkia cursor_image_;
  gfx::Size cursor_size_;
  gfx::Point cursor_hotspot_;
  gfx::Rect cursor_rect_;
  base::TimeTicks next_tick_time_;
  gfx::Vector2dF velocity_;
  gfx::Vector2dF responsive_velocity_;
  gfx::Vector2dF smooth_velocity_;
  sk_sp<cc::PaintFilter> motion_blur_filter_;
  gfx::Vector2dF motion_blur_offset_;
  SkMatrix motion_blur_matrix_;
  SkMatrix motion_blur_inverse_matrix_;
  std::unique_ptr<viz::DelayBasedTimeSource> time_source_;
  std::unique_ptr<base::Timer> stationary_timer_;
  base::RepeatingCallback<void(const gfx::Rect&, const gfx::Rect&, bool)>
      update_surface_callback_;
  SEQUENCE_CHECKER(paint_sequence_checker_);

  // UI thread state.
  std::unique_ptr<base::Thread> paint_thread_;
  ui::Compositor* compositor_ = nullptr;
  SEQUENCE_CHECKER(ui_sequence_checker_);
  base::WeakPtrFactory<CursorView> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(CursorView);
};

}  // namespace ash

#endif  // ASH_CURSOR_CURSOR_VIEW_H_
