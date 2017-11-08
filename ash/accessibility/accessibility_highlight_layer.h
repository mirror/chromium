// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_ACCESSIBILITY_ACCESSIBILITY_HIGHLIGHT_LAYER_H_
#define ASH_ACCESSIBILITY_ACCESSIBILITY_HIGHLIGHT_LAYER_H_

#include <vector>

#include "ash/accessibility/accessibility_layer.h"
#include "ash/ash_export.h"
#include "base/macros.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/geometry/rect.h"

namespace ash {

// A subclass of LayerDelegate that can highlight regions on the screen.
class ASH_EXPORT AccessibilityHighlightLayer : public AccessibilityLayer {
 public:
  AccessibilityHighlightLayer(AccessibilityLayerDelegate* delegate,
                              int red,
                              int green,
                              int blue);
  ~AccessibilityHighlightLayer() override;

  // Create the layer and update its bounds and position in the hierarchy.
  void Set(const std::vector<gfx::Rect>& rects);

  // AccessibilityLayer overrides:
  bool CanAnimate() const override;
  int GetInset() const override;

 private:
  // ui::LayerDelegate overrides:
  void OnPaintLayer(const ui::PaintContext& context) override;

  // The current rects to be highlighted.
  std::vector<gfx::Rect> rects_;

  int red_;
  int green_;
  int blue_;

  DISALLOW_COPY_AND_ASSIGN(AccessibilityHighlightLayer);
};

}  // namespace ash

#endif  // ASH_ACCESSIBILITY_ACCESSIBILITY_HIGHLIGHT_LAYER_H_
