// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_ACCESSIBILITY_FOCUS_RING_LAYER_H_
#define ASH_ACCESSIBILITY_FOCUS_RING_LAYER_H_

#include <memory>

#include "ash/accessibility/accessibility_layer.h"
#include "base/macros.h"
#include "base/optional.h"
#include "base/time/time.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/compositor/compositor_animation_observer.h"
#include "ui/compositor/layer_delegate.h"
#include "ui/gfx/geometry/rect.h"

namespace ash {

// A delegate interface implemented by the object that owns a FocusRingLayer.
class FocusRingLayerDelegate : public AccessibilityLayerDelegate {};

// FocusRingLayer draws a focus ring at a given global rectangle.
class FocusRingLayer : public AccessibilityLayer {
 public:
  explicit FocusRingLayer(FocusRingLayerDelegate* delegate);
  ~FocusRingLayer() override;

 private:
  // ui::LayerDelegate overrides:
  void OnPaintLayer(const ui::PaintContext& context) override;

  // The bounding rectangle of the focused object, in |root_window_|
  // coordinates.
  gfx::Rect focus_ring_;

  base::Optional<SkColor> custom_color_;

  DISALLOW_COPY_AND_ASSIGN(FocusRingLayer);
};

}  // namespace ash

#endif  // ASH_ACCESSIBILITY_FOCUS_RING_LAYER_H_
