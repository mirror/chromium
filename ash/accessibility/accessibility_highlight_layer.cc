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
    AccessibilityLayerDelegate* delegate)
    : AccessibilityLayer(delegate) {}

AccessibilityHighlightLayer::~AccessibilityHighlightLayer() {}

void AccessibilityHighlightLayer::Set(const std::vector<gfx::Rect>& rects,
                                      SkColor color) {
  rects_ = rects;
  highlight_color_ = color;

  // Calculate the bounds of all the rects together, that represents
  // the bounds of the full layer.
  gfx::Rect bounds;
  for (size_t i = 0; i < rects_.size(); ++i) {
    bounds.Union(rects_[i]);
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
  flags.setColor(highlight_color_);

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
