// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/overview/overview_window_mask.h"

#include "ui/aura/window.h"
#include "ui/compositor/paint_recorder.h"

namespace ash {

namespace {

// The amount of rounding on window edges in overview mode.
constexpr int kOverviewWindowRoundingDp = 4;

}  // namespace

OverviewWindowMask::OverviewWindowMask(aura::Window* window, bool inverted)
    : layer_(ui::LAYER_TEXTURED), inverted_(inverted), window_(window) {
  layer_.set_delegate(this);
  layer_.SetFillsBoundsOpaquely(false);
}

OverviewWindowMask::~OverviewWindowMask() {
  layer_.set_delegate(nullptr);
}

void OverviewWindowMask::OnPaintLayer(const ui::PaintContext& context) {
  cc::PaintFlags flags;
  flags.setAlpha(255);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kFill_Style);

  // The amount of round applied on the mask gets scaled as |window_| gets
  // transformed, so reverse the transform so the final scaled round matches
  // |kOverviewWindowRoundingDp|.
  SkScalar r_x = SkIntToScalar(kOverviewWindowRoundingDp);
  SkScalar r_y = r_x;
  if (window_) {
    const gfx::Vector2dF scale = window_->transform().Scale2d();
    r_x = SkIntToScalar(std::round(kOverviewWindowRoundingDp / scale.x()));
    r_y = SkIntToScalar(std::round(kOverviewWindowRoundingDp / scale.y()));
  }

  SkPath path;
  if (inverted_)
    path.setFillType(SkPath::kInverseWinding_FillType);
  SkScalar radii[8] = {r_x, r_y, r_x, r_y, r_x, r_y, r_x, r_y};
  gfx::Rect bounds(layer()->size());
  if (!crop_bounds_.IsEmpty()) {
    DCHECK(bounds.Contains(crop_bounds_));
    bounds = crop_bounds_;
  }
  bounds.Inset(0, top_inset_, 0, 0);
  path.addRoundRect(gfx::RectToSkRect(bounds), radii);

  ui::PaintRecorder recorder(context, layer()->size());
  recorder.canvas()->DrawPath(path, flags);
}

void OverviewWindowMask::OnDeviceScaleFactorChanged(
    float old_device_scale_factor,
    float new_device_scale_factor) {}

}  // namespace ash
