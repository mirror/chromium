// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/cursor/cursor_view.h"

#include "base/threading/thread.h"
#include "base/threading/thread_task_runner_handle.h"
#include "cc/paint/paint_canvas.h"
#include "ui/aura/window.h"
#include "ui/events/base_event_utils.h"
#include "ui/gfx/skia_util.h"
#include "ui/views/widget/widget.h"

namespace ash {
namespace {

// Amount of time without cursor movement before entering stationary state.
const int kStationaryDelayMs = 500;

// Clamp velocity to this value.
const float kVelocityMax = 5000.0f;

// Interpolation factor used to compute responsive velocity. Valid range
// is 0.0 to 1.0, where 1.0 takes only current velocity into account.
const float kResponsiveVelocityFactor = 0.75f;

// Interpolation factor used to compute smooth velocity. Valid range
// is 0.0 to 1.0, where 1.0 takes only current velocity into account.
const float kSmoothVelocityFactor = 0.25f;

// Interpolation factor used to compute cursor movement. Valid range
// is 0.0 to 1.0, where 1.0 takes only smooth velocity into account.
const float kMovementFactor = 0.25f;

// Minimum movement for motion blur to be added.
const float kMinimumMovementForMotionBlur = 2.0f;

// Clamp motion blur sigma to this value.
const float kSigmaMax = 48.0f;

// Offset relative to VSYNC at which to request a redraw.
const int kVSyncOffsetMs = -4;

gfx::Vector2dF InterpolateBetween(const gfx::Vector2dF& start,
                                  const gfx::Vector2dF& end,
                                  float x) {
  return start + gfx::ScaleVector2d(end - start, x);
}

}  // namespace

////////////////////////////////////////////////////////////////////////////////
// CursorView, public:

CursorView::CursorView(aura::Window* container,
                       const gfx::Point& initial_location,
                       bool is_motion_blur_enabled)
    : FastInkView(container),
      is_motion_blur_enabled_(is_motion_blur_enabled),
      ui_task_runner_(base::ThreadTaskRunnerHandle::Get()),
      new_location_(initial_location),
      stationary_timer_(new base::Timer(
          FROM_HERE,
          base::TimeDelta::FromMilliseconds(kStationaryDelayMs),
          base::BindRepeating(&CursorView::StationaryOnPaintThread,
                              base::Unretained(this)),
          /*is_repeating=*/false)),
      weak_ptr_factory_(this) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(ui_sequence_checker_);

  // Detach sequence checker for future usage on paint thread.
  DETACH_FROM_SEQUENCE(paint_sequence_checker_);

  // Create update surface callback that will be posted from paint thread
  // to UI thread.
  update_surface_callback_ = base::BindRepeating(
      &CursorView::UpdateSurface, weak_ptr_factory_.GetWeakPtr());

  base::Thread::Options thread_options;
  // Cursor uses a thread with |DISPLAY| priority for painting to avoid jank
  // when the system is under the high pressure.
  thread_options.priority = base::ThreadPriority::DISPLAY;
  paint_thread_.reset(new base::Thread("Cursor_PaintThread"));
  bool rv = paint_thread_->StartWithOptions(thread_options);
  DCHECK(rv);

  // Create transform used to convert cursor controller coordinates to screen
  // coordinates.
  rv = screen_to_buffer_transform_.GetInverse(&buffer_to_screen_transform_);
  DCHECK(rv);

  ui::CursorController::GetInstance()->AddCursorObserver(this);
}

CursorView::~CursorView() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(ui_sequence_checker_);

  if (compositor_) {
    compositor_->RemoveObserver(this);
    compositor_->vsync_manager()->RemoveObserver(this);
  }

  ui::CursorController::GetInstance()->RemoveCursorObserver(this);
}

void CursorView::SetCursorLocation(const gfx::Point& new_location) {
  {
    base::AutoLock lock(lock_);

    // Ignore location from UI when receiving location from cursor controller.
    if (location_from_cursor_controller_)
      return;

    if (new_location_ == new_location)
      return;
    new_location_ = new_location;
  }

  // Unretained is safe as |paint_thread_| is a member of this class.
  paint_thread_->task_runner()->PostTask(
      FROM_HERE, base::BindOnce(&CursorView::SetActiveOnPaintThread,
                                base::Unretained(this), true));
}

void CursorView::SetCursorImage(const gfx::ImageSkia& cursor_image,
                                const gfx::Size& cursor_size,
                                const gfx::Point& cursor_hotspot) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(ui_sequence_checker_);

  if (!compositor_) {
    compositor_ = GetWidget()->GetNativeWindow()->layer()->GetCompositor();
    compositor_->AddObserver(this);
    // AddObserver calls OnUpdateVSyncParameters.
    compositor_->vsync_manager()->AddObserver(this);
  }

  {
    base::AutoLock lock(lock_);

    new_cursor_image_ = cursor_image;
    new_cursor_size_ = cursor_size;
    new_cursor_hotspot_ = cursor_hotspot;
  }

  // Unretained is safe as |paint_thread_| is a member of this class.
  paint_thread_->task_runner()->PostTask(
      FROM_HERE, base::BindOnce(&CursorView::SetActiveOnPaintThread,
                                base::Unretained(this), true));
}

////////////////////////////////////////////////////////////////////////////////
// ui::CursorController::CursorObserver overrides:

void CursorView::OnCursorLocationChanged(const gfx::PointF& location) {
  gfx::PointF new_location_f = location;
  buffer_to_screen_transform_.TransformPoint(&new_location_f);
  gfx::Point new_location = gfx::ToRoundedPoint(new_location_f);

  {
    base::AutoLock lock(lock_);

    // Set |location_from_cursor_controller_| once we receive a location from
    // the cursor controller, so location from UI can be ignored.
    location_from_cursor_controller_ = true;

    if (new_location_ == new_location)
      return;
    new_location_ = new_location;
  }

  // Unretained is safe as |paint_thread_| is a member of this class.
  paint_thread_->task_runner()->PostTask(
      FROM_HERE, base::BindOnce(&CursorView::SetActiveOnPaintThread,
                                base::Unretained(this), true));
}

void CursorView::OnCompositingShuttingDown(ui::Compositor* compositor) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(ui_sequence_checker_);

  if (compositor_) {
    compositor_->vsync_manager()->RemoveObserver(this);
    compositor_->RemoveObserver(this);
    compositor_ = nullptr;
  }
}

////////////////////////////////////////////////////////////////////////////////
// ui::CompositorVSyncManager::Observer overrides:

void CursorView::OnUpdateVSyncParameters(base::TimeTicks timebase,
                                         base::TimeDelta interval) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(ui_sequence_checker_);

  // Unretained is safe as |paint_thread_| is a member of this class.
  paint_thread_->task_runner()->PostTask(
      FROM_HERE,
      base::BindOnce(&CursorView::SetTimebaseAndIntervalOnPaintThread,
                     base::Unretained(this), timebase, interval));
}

////////////////////////////////////////////////////////////////////////////////
// viz::DelayBasedTimeSourceClient overrides:

void CursorView::OnTimerTick() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(paint_sequence_checker_);

  gfx::Point old_location = location_;

  {
    base::AutoLock lock(lock_);

    location_ = new_location_;
    cursor_size_ = new_cursor_size_;
    cursor_image_ = new_cursor_image_;
    cursor_hotspot_ = new_cursor_hotspot_;
  }

  // Restart stationary timer if pointer location changed.
  if (location_ != old_location)
    stationary_timer_->Reset();

  base::TimeDelta interval = time_source_->Interval();
  // Compute velocity unless this is the first tick.
  if (time_source_->LastTickTime() == next_tick_time_) {
    // Velocity is pixels/second as interval might change.
    velocity_ = gfx::ScaleVector2d(old_location - location_,
                                   1.f / interval.InSecondsF());
    velocity_.SetToMin(gfx::Vector2dF(kVelocityMax, kVelocityMax));
  }

  // Save next tick time.
  next_tick_time_ = time_source_->NextTickTime();

  // Use "Complementary Filter" algorithm to determine velocity.
  // This allows us to be responsive in the short term and accurate
  // in the long term.
  responsive_velocity_ = InterpolateBetween(responsive_velocity_, velocity_,
                                            kResponsiveVelocityFactor);
  smooth_velocity_ =
      InterpolateBetween(smooth_velocity_, velocity_, kSmoothVelocityFactor);

  // Estimate movement over one time source (VSYNC) interval.
  gfx::Vector2dF movement =
      gfx::ScaleVector2d(InterpolateBetween(responsive_velocity_,
                                            smooth_velocity_, kMovementFactor),
                         interval.InSecondsF());

  float distance = movement.Length();
  if (is_motion_blur_enabled_ && distance >= kMinimumMovementForMotionBlur) {
    float sigma = std::min(distance / 3.f, kSigmaMax);

    // Create directional blur filter for |sigma|.
    motion_blur_filter_ = sk_make_sp<cc::BlurPaintFilter>(
        sigma, 0.f, SkBlurImageFilter::TileMode::kClampToBlack_TileMode,
        nullptr);

    // Compute blur offset.
    motion_blur_offset_ =
        gfx::ScaleVector2d(movement, std::ceil(sigma * 3.f) / distance);

    // Determine angle of movement.
    SkScalar angle = SkScalarATan2(SkFloatToScalar(movement.y()),
                                   SkFloatToScalar(movement.x()));
    SkScalar cos_angle = SkScalarCos(angle);
    SkScalar sin_angle = SkScalarSin(angle);

    // Create transformation matrices for blur space.
    motion_blur_matrix_.setSinCos(-sin_angle, cos_angle);
    motion_blur_inverse_matrix_.setSinCos(sin_angle, cos_angle);
  } else {
    motion_blur_filter_.reset();
    responsive_velocity_ = gfx::Vector2dF();
    smooth_velocity_ = gfx::Vector2dF();
    time_source_->SetActive(false);
  }

  // Damage is the union of old and new cursor rectangles.
  gfx::Rect damage_rect = cursor_rect_;
  cursor_rect_ = CalculateCursorRectOnPaintThread();
  damage_rect.Union(cursor_rect_);

  // Paint damaged area now that all parameters have been determined.
  {
    TRACE_EVENT1("ui", "CursorView::Paint", "damage_rect",
                 damage_rect.ToString());

    ScopedPaint paint(gpu_memory_buffer_.get(), screen_to_buffer_transform_,
                      damage_rect);
    cc::PaintCanvas* sk_canvas = paint.canvas().sk_canvas();
    sk_canvas->translate(SkIntToScalar(location_.x() - cursor_hotspot_.x()),
                         SkIntToScalar(location_.y() - cursor_hotspot_.y()));

    if (motion_blur_filter_) {
      sk_canvas->translate(SkIntToScalar(motion_blur_offset_.x()),
                           SkIntToScalar(motion_blur_offset_.y()));

      sk_canvas->concat(motion_blur_inverse_matrix_);
      SkRect blur_rect = SkRect::MakeWH(SkIntToScalar(cursor_size_.width()),
                                        SkIntToScalar(cursor_size_.height()));
      motion_blur_matrix_.mapRect(&blur_rect);
      cc::PaintFlags flags;
      flags.setImageFilter(motion_blur_filter_);
      sk_canvas->saveLayer(&blur_rect, &flags);
      sk_canvas->concat(motion_blur_matrix_);
      paint.canvas().DrawImageInt(cursor_image_, 0, 0);
      sk_canvas->restore();
    } else {
      // Fast path for when motion blur is not present.
      paint.canvas().DrawImageInt(cursor_image_, 0, 0);
    }
  }

  ui_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(update_surface_callback_, cursor_rect_, damage_rect,
                     /*auto_refresh=*/stationary_timer_->IsRunning()));
}

////////////////////////////////////////////////////////////////////////////////
// CursorView, private:

void CursorView::StationaryOnPaintThread() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(paint_sequence_checker_);

  stationary_timer_->Stop();
  ui_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(update_surface_callback_, cursor_rect_,
                                /*damage_rect=*/gfx::Rect(),
                                /*auto_refresh=*/false));
}

gfx::Rect CursorView::CalculateCursorRectOnPaintThread() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(paint_sequence_checker_);

  if (cursor_size_.IsEmpty())
    return gfx::Rect();

  SkRect cursor_rect = SkRect::MakeWH(SkIntToScalar(cursor_size_.width()),
                                      SkIntToScalar(cursor_size_.height()));

  if (motion_blur_filter_) {
    // Map curser rectangle to blur space.
    motion_blur_matrix_.mapRect(&cursor_rect);

    // Expand rectangle using current blur filter.
    cc::PaintFlags flags;
    flags.setImageFilter(motion_blur_filter_);
    DCHECK(flags.ToSkPaint().canComputeFastBounds());
    flags.ToSkPaint().computeFastBounds(cursor_rect, &cursor_rect);

    // Map rectangle back to cursor space.
    motion_blur_inverse_matrix_.mapRect(&cursor_rect);

    // Add motion blur offset.
    cursor_rect.offset(SkIntToScalar(motion_blur_offset_.x()),
                       SkIntToScalar(motion_blur_offset_.y()));
  }

  cursor_rect.offset(SkIntToScalar(location_.x() - cursor_hotspot_.x()),
                     SkIntToScalar(location_.y() - cursor_hotspot_.y()));

  return gfx::ToEnclosingRect(gfx::SkRectToRectF(cursor_rect));
}

void CursorView::SetActiveOnPaintThread(bool active) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(paint_sequence_checker_);

  if (time_source_)
    time_source_->SetActive(active);
}

void CursorView::SetTimebaseAndIntervalOnPaintThread(base::TimeTicks timebase,
                                                     base::TimeDelta interval) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(paint_sequence_checker_);

  // Create time source if it doesn't exist.
  if (!time_source_) {
    time_source_ = std::make_unique<viz::DelayBasedTimeSource>(
        paint_thread_->task_runner().get());
    time_source_->SetClient(this);
  }
  time_source_->SetTimebaseAndInterval(
      timebase + base::TimeDelta::FromMilliseconds(kVSyncOffsetMs), interval);
}

}  // namespace ash
