// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/accessibility/accessibility_highlight_layer.h"

#include "ash/shell.h"
#include "third_party/skia/include/core/SkPaint.h"
#include "third_party/skia/include/core/SkPath.h"
#include "ui/aura/window.h"
#include "ui/compositor/layer.h"
#include "ui/compositor/paint_recorder.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/wm/core/coordinate_conversion.h"

namespace ash {

namespace {

// Extra margin to add to the layer in DIP.
const int kLayerMargin = 1;

}  // namespace

AccessibilityHighlightLayer::AccessibilityHighlightLayer(
    AccessibilityLayerDelegate* delegate,
    int red,
    int green,
    int blue)
    : AccessibilityLayer(delegate), red_(red), green_(green), blue_(blue) {}

AccessibilityHighlightLayer::~AccessibilityHighlightLayer() {}

void AccessibilityHighlightLayer::Set(const std::vector<gfx::Rect>& rects) {
  rects_ = rects;

  gfx::Rect bounds;

  if (rects_.size() == 0) {
    bounds = gfx::Rect(0, 0, 0, 0);
  } else {
    // Calculate the bounds of all the rects together, that represents
    // the bounds of the full layer.
    gfx::Point top_left = gfx::Point(rects_[0].x(), rects_[0].y());
    gfx::Point bottom_right = rects_[0].bottom_right();
    for (size_t i = 1; i < rects_.size(); ++i) {
      top_left.SetToMin(gfx::Point(rects_[0].x(), rects_[0].y()));
      bottom_right.SetToMax(rects_[i].bottom_right());
    }
    bounds = gfx::Rect(top_left, gfx::Size(bottom_right.x() - top_left.x(),
                                           bottom_right.y() - top_left.y()));
  }

  int inset = kLayerMargin;
  bounds.Inset(-inset, -inset, -inset, -inset);

  display::Display display =
      display::Screen::GetScreen()->GetDisplayMatching(bounds);
  aura::Window* root_window = Shell::GetRootWindowForDisplayId(display.id());
  ::wm::ConvertRectFromScreen(root_window, &bounds);
  CreateOrUpdateLayer(root_window, "AccessibilityHighlight", bounds);
}

bool AccessibilityHighlightLayer::CanAnimate() const {
  return false;
}

int AccessibilityHighlightLayer::GetInset() const {
  return kLayerMargin;
}

void AccessibilityHighlightLayer::OnPaintLayer(
    const ui::PaintContext& context) {
  ui::PaintRecorder recorder(context, layer_->size());

  cc::PaintFlags flags;
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kFill_Style);
  flags.setColor(SkColorSetRGB(red_, green_, blue_));

  gfx::Rect r = layer_->bounds();

  for (size_t i = 0; i < rects_.size(); ++i) {
    gfx::Rect rect = rects_[i];
    // Offset the rect based on where the layer is on the screen.
    rect.Offset(-r.OffsetFromOrigin());
    // Add a little bit of margin to the drawn box.
    rect.Inset(-kLayerMargin, -kLayerMargin, -kLayerMargin, -kLayerMargin);
    recorder.canvas()->DrawRect(rect, flags);
  }
}

}  // namespace ash
