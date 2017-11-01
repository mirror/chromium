// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/elements/button.h"

#include "base/memory/ptr_util.h"
#include "chrome/browser/vr/color_scheme.h"
#include "chrome/browser/vr/elements/rect.h"
#include "chrome/browser/vr/elements/ui_element.h"
#include "chrome/browser/vr/elements/ui_element_name.h"
#include "chrome/browser/vr/elements/vector_icon.h"
#include "chrome/browser/vr/ui_scene_constants.h"

#include "ui/gfx/geometry/point_f.h"

namespace vr {

namespace {

constexpr float kIconScaleFactor = 0.5;
constexpr float kBackgroundZOffset = -kTextureOffset;
constexpr float kForegroundZOffset = kTextureOffset;
constexpr float kBackgroundZOffsetHover = kTextureOffset;
constexpr float kForegroundZOffsetHover = 3 * kTextureOffset;

}  // namespace

Button::Button(base::Callback<void()> click_handler,
               int draw_phase,
               float width,
               float height,
               const gfx::VectorIcon& icon)
    : click_handler_(click_handler) {
  set_draw_phase(draw_phase);
  SetSize(width, height);

  auto background = base::MakeUnique<Rect>();
  background->set_name(kNone);
  background->set_draw_phase(draw_phase);
  background->SetSize(width, height);
  background->SetTransitionedProperties({TRANSFORM});
  background->set_corner_radius(width / 2);
  background->set_hit_testable(false);
  background->SetTranslate(0.0, 0.0, kBackgroundZOffset);
  background_ = background.get();
  AddChild(std::move(background));

  auto vector_icon = base::MakeUnique<VectorIcon>(512);
  vector_icon->set_name(kNone);
  vector_icon->SetIcon(icon);
  vector_icon->set_draw_phase(draw_phase);
  vector_icon->SetSize(width * kIconScaleFactor, height * kIconScaleFactor);
  vector_icon->SetTransitionedProperties({TRANSFORM});
  vector_icon->set_hit_testable(false);
  vector_icon->SetTranslate(0.0, 0.0, kForegroundZOffset);
  foreground_ = vector_icon.get();
  AddChild(std::move(vector_icon));
}

Button::~Button() = default;

void Button::OnHoverEnter(const gfx::PointF& position) {
  OnStateUpdated(position);
}

void Button::OnHoverLeave() {
  OnStateUpdated(gfx::PointF(std::numeric_limits<float>::max(),
                             std::numeric_limits<float>::max()));
}

void Button::OnMove(const gfx::PointF& position) {
  OnStateUpdated(position);
}

void Button::OnButtonDown(const gfx::PointF& position) {
  down_ = true;
  OnStateUpdated(position);
}

void Button::OnButtonUp(const gfx::PointF& position) {
  down_ = false;
  OnStateUpdated(position);
  if (HitTest(position))
    click_handler_.Run();
}

bool Button::HitTest(const gfx::PointF& point) const {
  return (point - gfx::PointF(0.5, 0.5)).LengthSquared() < 0.25;
}

void Button::SetBackgroundColor(SkColor color) {
  background_color_ = color;
  if (!hovered_ && !pressed_)
    background_->SetColor(background_color_);
}

void Button::SetBackgroundColorHovered(SkColor color) {
  background_color_hovered_ = color;
  if (hovered_ && !pressed_)
    background_->SetColor(background_color_hovered_);
}

void Button::SetBackgroundColorPressed(SkColor color) {
  background_color_pressed_ = color;
  if (pressed_)
    background_->SetColor(background_color_pressed_);
}

void Button::SetVectorIconColor(SkColor color) {
  foreground_color_ = color;
  foreground_->SetColor(foreground_color_);
}

void Button::OnStateUpdated(const gfx::PointF& position) {
  hovered_ = HitTest(position);
  pressed_ = hovered_ ? down_ : false;

  if (hovered_) {
    background_->SetTranslate(0.0, 0.0, kBackgroundZOffsetHover);
    foreground_->SetTranslate(0.0, 0.0, kForegroundZOffsetHover);
  } else {
    background_->SetTranslate(0.0, 0.0, kBackgroundZOffset);
    foreground_->SetTranslate(0.0, 0.0, kForegroundZOffset);
  }

  if (pressed_) {
    background_->SetColor(background_color_pressed_);
  } else if (hovered_) {
    background_->SetColor(background_color_hovered_);
  } else {
    background_->SetColor(background_color_);
  }
}

}  // namespace vr
