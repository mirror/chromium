// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/highlighter/highlighter_view.h"

#include "third_party/skia/include/core/SkColor.h"
#include "third_party/skia/include/core/SkTypes.h"
#include "ui/aura/window.h"
#include "ui/compositor/scoped_layer_animation_settings.h"
#include "ui/events/base_event_utils.h"
#include "ui/gfx/canvas.h"
#include "ui/views/widget/widget.h"

namespace ash {
namespace {

// Variables for rendering the highlighter. Sizes in DIP.
const SkColor kHighlighterColor = SkColorSetRGB(0x42, 0x85, 0xF4);
constexpr int kHighlighterOpacity = 0xCC;
constexpr float kPenTipWidth = 4;
constexpr float kPenTipHeight = 14;
constexpr int kOutsetForAntialiasing = 1;

constexpr float kStrokeScale = 1.2;

constexpr int kStrokeFadeoutDelayMs = 500;
constexpr int kStrokeFadeoutDurationMs = 500;
constexpr int kStrokeScaleDurationMs = 300;

gfx::RectF GetPenTipRect(const gfx::PointF& p) {
  return gfx::RectF(p.x() - kPenTipWidth / 2, p.y() - kPenTipHeight / 2,
                    kPenTipWidth, kPenTipHeight);
}

gfx::Rect InflateDamageRect(const gfx::Rect& r) {
  gfx::Rect inflated = r;
  inflated.Inset(-kOutsetForAntialiasing - (int)(kPenTipWidth / 2),
                 -kOutsetForAntialiasing - (int)(kPenTipHeight / 2));
  return inflated;
}

// A highlighter segment is best imagined as a result of a rectangle
// being dragged along a straight line, while keeping its orientation.
// r1 is the start position of the rectangle and r2 is the final position.
void DrawSegment(gfx::Canvas& canvas,
                 const gfx::PointF& p1,
                 const gfx::PointF& p2,
                 const cc::PaintFlags& flags) {
  if (p1.x() > p2.x()) {
    DrawSegment(canvas, p2, p1, flags);
    return;
  }

  const gfx::RectF r1 = GetPenTipRect(p1);
  const gfx::RectF r2 = GetPenTipRect(p2);

  SkPath path;
  path.moveTo(r1.x(), r1.y());
  if (r1.y() < r2.y())
    path.lineTo(r1.right(), r1.y());
  else
    path.lineTo(r2.x(), r2.y());
  path.lineTo(r2.right(), r2.y());
  path.lineTo(r2.right(), r2.bottom());
  if (r1.y() < r2.y())
    path.lineTo(r2.x(), r2.bottom());
  else
    path.lineTo(r1.right(), r1.bottom());
  path.lineTo(r1.x(), r1.bottom());
  path.lineTo(r1.x(), r1.y());
  canvas.DrawPath(path, flags);
}

}  // namespace

const SkColor HighlighterView::kPenColor =
    SkColorSetA(kHighlighterColor, kHighlighterOpacity);

const gfx::SizeF HighlighterView::kPenTipSize(kPenTipWidth, kPenTipHeight);

HighlighterView::HighlighterView(base::TimeDelta presentation_delay,
                                 aura::Window* root_window)
    : FastInkView(root_window),
      points_(base::TimeDelta()),
      predicted_points_(base::TimeDelta()),
      presentation_delay_(presentation_delay) {}

HighlighterView::~HighlighterView() {}

void HighlighterView::AddNewPoint(const gfx::PointF& point,
                                  const base::TimeTicks& time) {
  TRACE_EVENT1("ui", "HighlighterView::AddNewPoint", "point", point.ToString());

  // The new segment needs to be drawn.
  if (!points_.IsEmpty()) {
    UpdateDamageRect(InflateDamageRect(gfx::ToEnclosingRect(
        gfx::BoundingRect(points_.GetNewest().location, point))));
  }

  // Previous prediction needs to be erased.
  if (!predicted_points_.IsEmpty())
    UpdateDamageRect(InflateDamageRect(predicted_points_.GetBoundingBox()));

  points_.AddPoint(point, time);

  base::TimeTicks current_time = ui::EventTimeForNow();
  predicted_points_.Predict(
      points_, current_time, presentation_delay_,
      GetWidget()->GetNativeView()->GetBoundsInScreen().size());

  // New prediction needs to be drawn.
  if (!predicted_points_.IsEmpty())
    UpdateDamageRect(InflateDamageRect(predicted_points_.GetBoundingBox()));

  RequestRedraw();
}

gfx::Rect HighlighterView::GetBoundingBox() const {
  gfx::Rect rect = points_.GetBoundingBox();
  rect.Union(predicted_points_.GetBoundingBox());
  return rect;
}

std::vector<gfx::PointF> HighlighterView::GetTrace() const {
  std::vector<gfx::PointF> result;
  for (auto p : points_.points())
    result.push_back(p.location);
  for (auto p : predicted_points_.points())
    result.push_back(p.location);
  return result;
}

void HighlighterView::Animate(const gfx::Point& pivot,
                              AnimationMode animation_mode,
                              const base::Closure& done) {
  animation_timer_.reset(new base::Timer(
      FROM_HERE, base::TimeDelta::FromMilliseconds(kStrokeFadeoutDelayMs),
      base::Bind(&HighlighterView::FadeOut, base::Unretained(this), pivot,
                 animation_mode, done),
      false));
  animation_timer_->Reset();
}

void HighlighterView::FadeOut(const gfx::Point& pivot,
                              AnimationMode animation_mode,
                              const base::Closure& done) {
  ui::Layer* layer = GetWidget()->GetLayer();

  {
    ui::ScopedLayerAnimationSettings settings(layer->GetAnimator());
    settings.SetTransitionDuration(
        base::TimeDelta::FromMilliseconds(kStrokeFadeoutDurationMs));
    settings.SetTweenType(gfx::Tween::LINEAR_OUT_SLOW_IN);

    layer->SetOpacity(0);
  }

  if (animation_mode != AnimationMode::kFadeout) {
    ui::ScopedLayerAnimationSettings settings(layer->GetAnimator());
    settings.SetTransitionDuration(
        base::TimeDelta::FromMilliseconds(kStrokeScaleDurationMs));
    settings.SetTweenType(gfx::Tween::LINEAR_OUT_SLOW_IN);

    const float scale = animation_mode == AnimationMode::kInflate
                            ? kStrokeScale
                            : (1 / kStrokeScale);

    gfx::Transform transform;
    transform.Translate(pivot.x() * (1 - scale), pivot.y() * (1 - scale));
    transform.Scale(scale, scale);

    layer->SetTransform(transform);
  }

  animation_timer_.reset(new base::Timer(
      FROM_HERE, base::TimeDelta::FromMilliseconds(kStrokeFadeoutDurationMs),
      done, false));
  animation_timer_->Reset();
}

void HighlighterView::OnRedraw(gfx::Canvas& canvas,
                               const gfx::Vector2d& offset) {
  int num_points =
      points_.GetNumberOfPoints() + predicted_points_.GetNumberOfPoints();
  if (num_points < 2)
    return;

  gfx::Rect clip_rect;
  if (!canvas.GetClipBounds(&clip_rect))
    return;

  cc::PaintFlags flags;
  flags.setStyle(cc::PaintFlags::kFill_Style);
  flags.setAntiAlias(true);
  flags.setColor(kPenColor);
  flags.setBlendMode(SkBlendMode::kSrc);

  gfx::PointF previous_point;

  for (int i = 0; i < num_points; ++i) {
    gfx::PointF current_point;
    if (i < points_.GetNumberOfPoints()) {
      current_point = points_.points()[i].location - offset;
    } else {
      current_point =
          predicted_points_.points()[i - points_.GetNumberOfPoints()].location -
          offset;
    }

    if (i != 0) {
      gfx::Rect damage_rect = InflateDamageRect(gfx::ToEnclosingRect(
          gfx::BoundingRect(previous_point, current_point)));
      // Only draw the segment if it is touching the clip rect.
      if (clip_rect.Intersects(damage_rect)) {
        DrawSegment(canvas, previous_point, current_point, flags);
      }
    }

    previous_point = current_point;
  }
}

}  // namespace ash
