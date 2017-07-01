// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/highlighter/highlighter_result_view.h"

#include "ash/public/cpp/shell_window_ids.h"
#include "ash/shell.h"
#include "ui/compositor/paint_recorder.h"
#include "ui/compositor/scoped_layer_animation_settings.h"
#include "ui/views/widget/widget.h"

namespace ash {

namespace {

// Variables for rendering the highlight result. Sizes in DIP.
const float kCornerCircleRadius = 6;
const float kStrokeFillWidth = 2;
const float kStrokeOutlineWidth = 1;
const float kOutsetForAntialiasing = 1;
const float kResultLayerMargin = kOutsetForAntialiasing + kCornerCircleRadius;

const int kInnerFillOpacity = 0x0D;
const int kInnerFillColor = SkColorSetRGB(0x00, 0x00, 0x00);

const int kStrokeFillOpacity = 0xFF;
const int kStrokeFillColor = SkColorSetRGB(0xFF, 0xFF, 0xFF);

const int kStrokeOutlineOpacity = 0x14;
const int kStrokeOutlineColor = SkColorSetRGB(0x00, 0x00, 0x00);

const int kCornerCircleOpacity = 0xFF;
const int kCornerCircleColorLT = SkColorSetRGB(0x42, 0x85, 0xF4);
const int kCornerCircleColorRT = SkColorSetRGB(0xEA, 0x43, 0x35);
const int kCornerCircleColorLB = SkColorSetRGB(0x34, 0xA8, 0x53);
const int kCornerCircleColorRB = SkColorSetRGB(0xFB, 0xBC, 0x05);

const int kResultFadeinDelayMs = 600;
const int kResultFadeinDurationMs = 400;
const int kResultFadeoutDelayMs = 500;
const int kResultFadeoutDurationMs = 200;

const int kResultInPlaceFadeinDelayMs = 500;
const int kResultInPlaceFadeinDurationMs = 500;

const float kInitialScale = 1.2;

class ResultLayer : public ui::Layer, public ui::LayerDelegate {
 public:
  ResultLayer(const gfx::Rect& bounds);

 private:
  void OnDelegatedFrameDamage(const gfx::Rect& damage_rect_in_dip) override {}

  void OnDeviceScaleFactorChanged(float device_scale_factor) override {}

  void OnPaintLayer(const ui::PaintContext& context) override;

  void DrawVerticalBar(gfx::Canvas& canvas,
                       float x,
                       float y,
                       float height,
                       cc::PaintFlags& flags);
  void DrawHorizontalBar(gfx::Canvas& canvas,
                         float x,
                         float y,
                         float width,
                         cc::PaintFlags& flags);

  DISALLOW_COPY_AND_ASSIGN(ResultLayer);
};

ResultLayer::ResultLayer(const gfx::Rect& box) {
  set_name("HighlighterResultView:ResultLayer");
  gfx::Rect bounds = box;
  bounds.Inset(-kResultLayerMargin, -kResultLayerMargin);
  SetBounds(bounds);
  SetFillsBoundsOpaquely(false);
  SetMasksToBounds(false);
  set_delegate(this);
}

void ResultLayer::OnPaintLayer(const ui::PaintContext& context) {
  ui::PaintRecorder recorder(context, size());
  gfx::Canvas& canvas = *recorder.canvas();

  cc::PaintFlags flags;
  flags.setStyle(cc::PaintFlags::kFill_Style);
  flags.setAntiAlias(true);
  flags.setBlendMode(SkBlendMode::kSrc);

  const float left = kResultLayerMargin;
  const float right = size().width() - kResultLayerMargin;
  const float width = right - left;

  const float top = kResultLayerMargin;
  const float bottom = size().height() - kResultLayerMargin;
  const float height = bottom - top;

  flags.setColor(SkColorSetA(kInnerFillColor, kInnerFillOpacity));
  canvas.DrawRect(gfx::RectF(left, top, width, height), flags);

  DrawVerticalBar(canvas, left, top, height, flags);
  DrawVerticalBar(canvas, right, top, height, flags);
  DrawHorizontalBar(canvas, left, top, width, flags);
  DrawHorizontalBar(canvas, left, bottom, width, flags);

  flags.setColor(SkColorSetA(kCornerCircleColorLT, kCornerCircleOpacity));
  canvas.DrawCircle(gfx::PointF(left, top), kCornerCircleRadius, flags);

  flags.setColor(SkColorSetA(kCornerCircleColorRT, kCornerCircleOpacity));
  canvas.DrawCircle(gfx::PointF(right, top), kCornerCircleRadius, flags);

  flags.setColor(SkColorSetA(kCornerCircleColorLB, kCornerCircleOpacity));
  canvas.DrawCircle(gfx::PointF(right, bottom), kCornerCircleRadius, flags);

  flags.setColor(SkColorSetA(kCornerCircleColorRB, kCornerCircleOpacity));
  canvas.DrawCircle(gfx::PointF(left, bottom), kCornerCircleRadius, flags);
}

void ResultLayer::DrawVerticalBar(gfx::Canvas& canvas,
                                  float x,
                                  float y,
                                  float height,
                                  cc::PaintFlags& flags) {
  const float xFill = x - kStrokeFillWidth / 2;
  const float xOutlineLeft = xFill - kStrokeOutlineWidth;
  const float xOutlineRight = xFill + kStrokeFillWidth;

  flags.setColor(SkColorSetA(kStrokeFillColor, kStrokeFillOpacity));
  canvas.DrawRect(gfx::RectF(xFill, y, kStrokeFillWidth, height), flags);

  flags.setColor(SkColorSetA(kStrokeOutlineColor, kStrokeOutlineOpacity));
  canvas.DrawRect(gfx::RectF(xOutlineLeft, y, kStrokeOutlineWidth, height),
                  flags);
  canvas.DrawRect(gfx::RectF(xOutlineRight, y, kStrokeOutlineWidth, height),
                  flags);
}

void ResultLayer::DrawHorizontalBar(gfx::Canvas& canvas,
                                    float x,
                                    float y,
                                    float width,
                                    cc::PaintFlags& flags) {
  const float yFill = y - kStrokeFillWidth / 2;
  const float yOutlineLeft = yFill - kStrokeOutlineWidth;
  const float yOutlineRight = yFill + kStrokeFillWidth;

  flags.setColor(SkColorSetA(kStrokeFillColor, kStrokeFillOpacity));
  canvas.DrawRect(gfx::RectF(x, yFill, width, kStrokeFillWidth), flags);

  flags.setColor(SkColorSetA(kStrokeOutlineColor, kStrokeOutlineOpacity));
  canvas.DrawRect(gfx::RectF(x, yOutlineLeft, width, kStrokeOutlineWidth),
                  flags);
  canvas.DrawRect(gfx::RectF(x, yOutlineRight, width, kStrokeOutlineWidth),
                  flags);
}

}  // namespace

HighlighterResultView::HighlighterResultView(aura::Window* root_window) {
  widget_.reset(new views::Widget);

  views::Widget::InitParams params;
  params.type = views::Widget::InitParams::TYPE_WINDOW_FRAMELESS;
  params.name = "HighlighterResult";
  params.accept_events = false;
  params.activatable = views::Widget::InitParams::ACTIVATABLE_NO;
  params.ownership = views::Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
  params.opacity = views::Widget::InitParams::TRANSLUCENT_WINDOW;
  params.parent =
      Shell::GetContainer(root_window, kShellWindowId_OverlayContainer);
  params.layer_type = ui::LAYER_SOLID_COLOR;

  widget_->Init(params);
  widget_->Show();
  widget_->SetContentsView(this);
  widget_->SetBounds(root_window->GetBoundsInScreen());
  set_owned_by_client();
}

void HighlighterResultView::AnimateInPlace(const gfx::Rect& bounds,
                                           SkColor color) {
  ui::Layer* layer = widget_->GetLayer();

  // A solid transparent rectangle.
  result_layer_.reset(new ui::Layer(ui::LAYER_SOLID_COLOR));
  result_layer_->set_name("HighlighterResultView:SOLID_LAYER");
  result_layer_->SetBounds(bounds);
  result_layer_->SetFillsBoundsOpaquely(false);
  result_layer_->SetMasksToBounds(false);
  result_layer_->SetColor(color);

  layer->Add(result_layer_.get());

  ScheduleFadeIn(kResultInPlaceFadeinDelayMs, kResultInPlaceFadeinDurationMs);
}

void HighlighterResultView::AnimateDeflate(const gfx::Rect& bounds) {
  ui::Layer* layer = widget_->GetLayer();

  result_layer_.reset(new ResultLayer(bounds));
  layer->Add(result_layer_.get());

  gfx::Transform transform;
  const gfx::Point pivot = bounds.CenterPoint();
  transform.Translate(pivot.x() * (1 - kInitialScale),
                      pivot.y() * (1 - kInitialScale));
  transform.Scale(kInitialScale, kInitialScale);
  layer->SetTransform(transform);

  ScheduleFadeIn(kResultFadeinDelayMs, kResultFadeinDurationMs);
}

void HighlighterResultView::ScheduleFadeIn(long delay_ms, long duration_ms) {
  ui::Layer* layer = widget_->GetLayer();

  layer->SetOpacity(0);

  animation_timer_.reset(
      new base::Timer(FROM_HERE, base::TimeDelta::FromMilliseconds(delay_ms),
                      base::Bind(&HighlighterResultView::FadeIn,
                                 base::Unretained(this), duration_ms),
                      false));
  animation_timer_->Reset();
}

void HighlighterResultView::FadeIn(long duration_ms) {
  ui::Layer* layer = widget_->GetLayer();

  {
    ui::ScopedLayerAnimationSettings settings(layer->GetAnimator());
    settings.SetTransitionDuration(
        base::TimeDelta::FromMilliseconds(duration_ms));
    settings.SetTweenType(gfx::Tween::LINEAR_OUT_SLOW_IN);
    layer->SetOpacity(1);
  }

  {
    ui::ScopedLayerAnimationSettings settings(layer->GetAnimator());
    settings.SetTransitionDuration(
        base::TimeDelta::FromMilliseconds(duration_ms));
    settings.SetTweenType(gfx::Tween::LINEAR_OUT_SLOW_IN);

    gfx::Transform transform;
    transform.Scale(1, 1);
    transform.Translate(0, 0);
    layer->SetTransform(transform);
  }

  animation_timer_.reset(new base::Timer(
      FROM_HERE,
      base::TimeDelta::FromMilliseconds(kResultFadeinDurationMs +
                                        kResultFadeoutDelayMs),
      base::Bind(&HighlighterResultView::FadeOut, base::Unretained(this)),
      false));
  animation_timer_->Reset();
}

void HighlighterResultView::FadeOut() {
  ui::Layer* layer = widget_->GetLayer();

  {
    ui::ScopedLayerAnimationSettings settings(layer->GetAnimator());
    settings.SetTransitionDuration(
        base::TimeDelta::FromMilliseconds(kResultFadeoutDurationMs));
    settings.SetTweenType(gfx::Tween::LINEAR_OUT_SLOW_IN);
    layer->SetOpacity(0);
  }
}

}  // namespace ash
