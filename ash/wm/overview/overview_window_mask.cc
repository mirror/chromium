// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/overview/overview_window_mask.h"

#include "ui/compositor/paint_recorder.h"

namespace ash {

namespace {

// The amount of rounding on window edges in overview mode.
constexpr int kOverviewWindowRoundingDp = 4;

}  // namespace

OverviewWindowMask::OverviewWindowMask(const gfx::Vector2dF& scale,
                                       bool is_inside_mask)
    : layer_(ui::LAYER_TEXTURED),
      is_inside_mask_(is_inside_mask),
      scale_(scale) {
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

  // This mask is meant to be applied to a layer which may be affected by a
  // transform. The amount of round applied on the mask will also gets scaled,
  // so reverse the scale defined by |scale_| so the final scaled round matches
  // |kOverviewWindowRoundingDp|.
  const SkScalar r_x =
      SkIntToScalar(std::round(kOverviewWindowRoundingDp / scale_.x()));
  const SkScalar r_y =
      SkIntToScalar(std::round(kOverviewWindowRoundingDp / scale_.y()));

  SkPath path;
  if (is_inside_mask_)
    path.setFillType(SkPath::kInverseWinding_FillType);
  SkScalar radii[8] = {r_x, r_y, r_x, r_y, r_x, r_y, r_x, r_y};
  gfx::Rect bounds(layer()->size());
  if (!crop_bounds_.IsEmpty()) {
    if (bounds.Contains(crop_bounds_))
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
