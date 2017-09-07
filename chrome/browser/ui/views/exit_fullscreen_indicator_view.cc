// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/exit_fullscreen_indicator_view.h"

#include "cc/paint/paint_flags.h"
#include "components/vector_icons/vector_icons.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/views/background.h"
#include "ui/views/controls/image_view.h"

namespace {

// Partially-transparent background color.
const SkColor kBackgroundColor = SkColorSetARGB(0xcc, 0x28, 0x2c, 0x32);

const int kIndicatorDiameter = 48;

constexpr gfx::Size kPreferredSize =
    gfx::Size(kIndicatorDiameter, kIndicatorDiameter);

class CircleBackground : public views::Background {
 public:
  explicit CircleBackground(SkColor color);
  ~CircleBackground() override;

  void Paint(gfx::Canvas* canvas, views::View* view) const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(CircleBackground);
};

CircleBackground::CircleBackground(SkColor color) {
  SetNativeControlColor(color);
}

CircleBackground::~CircleBackground() {}

void CircleBackground::Paint(gfx::Canvas* canvas, views::View* view) const {
  const gfx::Rect& bounds = view->bounds();
  int radius = std::min(bounds.size().width(), bounds.size().height()) / 2;
  cc::PaintFlags flags;
  flags.setAntiAlias(true);
  flags.setColor(get_color());
  flags.setStyle(cc::PaintFlags::kFill_Style);
  canvas->DrawCircle(bounds.CenterPoint(), radius, flags);
}

}  // namespace

ExitFullscreenIndicatorView::ExitFullscreenIndicatorView() {
  SetBackground(base::MakeUnique<CircleBackground>(kBackgroundColor));
  SetPreferredSize(kPreferredSize);
  close_image_view_ = new views::ImageView();
  close_image_view_->SetImage(
      gfx::CreateVectorIcon(vector_icons::kIcCloseIcon, SK_ColorWHITE));
  AddChildView(close_image_view_);
}

ExitFullscreenIndicatorView::~ExitFullscreenIndicatorView() {}

void ExitFullscreenIndicatorView::Layout() {
  gfx::Size image_size = close_image_view_->GetImage().size();
  gfx::Size indicator_size = bounds().size();

  if (indicator_size.IsEmpty()) {
    // Not ready to layout yet.
    return;
  }

  DCHECK(image_size.width() < indicator_size.width() &&
         image_size.height() < indicator_size.height());

  gfx::Point image_origin((indicator_size.width() - image_size.width()) / 2,
                          (indicator_size.height() - image_size.height()) / 2);
  close_image_view_->SetBoundsRect({image_origin, image_size});
}
