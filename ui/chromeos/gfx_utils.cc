// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/chromeos/gfx_utils.h"

#include "cc/paint/paint_flags.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/image/canvas_image_source.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/image/image_skia_operations.h"
#include "ui/gfx/shadow_value.h"

namespace ui {

namespace {

// The background image source for badge.
class BadgeBackgroundImageSource : public gfx::CanvasImageSource {
 public:
  explicit BadgeBackgroundImageSource(int size)
      : CanvasImageSource(gfx::Size(size, size), false),
        radius_(static_cast<float>(size / 2)) {}
  ~BadgeBackgroundImageSource() override = default;

 private:
  // gfx::CanvasImageSource overrides:
  void Draw(gfx::Canvas* canvas) override {
    cc::PaintFlags flags;
    flags.setColor(SK_ColorWHITE);
    flags.setAntiAlias(true);
    flags.setStyle(cc::PaintFlags::kFill_Style);
    canvas->DrawCircle(gfx::PointF(radius_, radius_), radius_, flags);
  }

  const float radius_;

  DISALLOW_COPY_AND_ASSIGN(BadgeBackgroundImageSource);
};

}  // namespace

gfx::ImageSkia CreateBadgeImage(const gfx::ImageSkia& image_source) {
  const int size = kBadgeBackgroundRadius * 2;
  gfx::ImageSkia background(std::make_unique<BadgeBackgroundImageSource>(size),
                            gfx::Size(size, size));
  gfx::ImageSkia icon_with_background =
      gfx::ImageSkiaOperations::CreateSuperimposedImage(background,
                                                        image_source);

  gfx::ShadowValues shadow_values;
  shadow_values.push_back(
      gfx::ShadowValue(gfx::Vector2d(0, 1), 0, SkColorSetARGB(0x33, 0, 0, 0)));
  shadow_values.push_back(
      gfx::ShadowValue(gfx::Vector2d(0, 1), 2, SkColorSetARGB(0x33, 0, 0, 0)));
  return gfx::ImageSkiaOperations::CreateImageWithDropShadow(
      icon_with_background, shadow_values);
}

}  // namespace ui
