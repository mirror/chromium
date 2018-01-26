// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WM_OVERVIEW_OVERVIEW_WINDOW_MASK_H_
#define ASH_WM_OVERVIEW_OVERVIEW_WINDOW_MASK_H_

#include "base/macros.h"
#include "ui/compositor/layer.h"
#include "ui/compositor/layer_delegate.h"

namespace aura {
class Window;
}

namespace ash {

// OverviewWindowMask is applied to overview windows to give them rounded edges
// while they are in overview mode.
class OverviewWindowMask : public ui::LayerDelegate {
 public:
  OverviewWindowMask(aura::Window* window, bool inverted);

  ~OverviewWindowMask() override;

  void set_top_inset(int top_inset) { top_inset_ = top_inset; }
  void set_crop_bounds(const gfx::Rect& crop_bounds) {
    mask_bounds_ = crop_bounds;
  }
  ui::Layer* layer() { return &layer_; }

 private:
  // ui::LayerDelegate:
  void OnPaintLayer(const ui::PaintContext& context) override;
  void OnDeviceScaleFactorChanged(float old_device_scale_factor,
                                  float new_device_scale_factor) override;

  ui::Layer layer_;
  // Specifies the amount off the top to crop. This is so headers do not get
  // masked.
  int top_inset_ = 0;
  // Specifies the area which to apply the rounded rect mask. If this is empty,
  // the mask will take up the bounds of |layer_|.
  gfx::Rect crop_bounds_;
  // If true, the inside of the round rect is masked as opposed to the outside.
  bool inverted_;
  // Pointer to the window of which this is a mask to. May be null if the mask
  // is not applied to a window layer.
  aura::Window* window_;

  DISALLOW_COPY_AND_ASSIGN(OverviewWindowMask);
};

}  // namespace ash

#endif  // ASH_WM_OVERVIEW_OVERVIEW_WINDOW_MASK_H_
