// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/fab_icon_view.h"

#include <algorithm>

#include "cc/paint/paint_flags.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/widget/widget.h"

namespace {

// Partially-transparent background color.
const SkColor kBackgroundColor = SkColorSetARGB(0xcc, 0x28, 0x2c, 0x32);

const int kFabDiameter = 48;

constexpr gfx::Size kPreferredSize = gfx::Size(kFabDiameter, kFabDiameter);

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

FabIconView::FabIconView() {
  SetBorder(views::NullBorder());
  SetBackground(base::MakeUnique<CircleBackground>(kBackgroundColor));
  SetPreferredSize(kPreferredSize);
  close_image_view_ = new views::ImageView();
  AddChildView(close_image_view_);
}

FabIconView::~FabIconView() {}

void FabIconView::SetIcon(const gfx::VectorIcon& icon) {
  close_image_view_->SetImage(gfx::CreateVectorIcon(icon, SK_ColorWHITE));
  Layout();
}

void FabIconView::Layout() {
  if (close_image_view_->GetImage().isNull()) {
    return;
  }

  gfx::Size image_size = close_image_view_->GetImage().size();
  gfx::Size fab_size = bounds().size();
  if (image_size.width() > fab_size.width() ||
      image_size.height() > fab_size.height()) {
    // Image can't fit the background. Make its size zero.
    close_image_view_->SetBounds(0, 0, 0, 0);
    return;
  }
  gfx::Point image_origin((fab_size.width() - image_size.width()) / 2,
                          (fab_size.height() - image_size.height()) / 2);
  close_image_view_->SetBoundsRect({image_origin, image_size});
}

// static
views::Widget* FabIconView::CreatePopupWidget(gfx::NativeView parent_view,
                                              FabIconView* view,
                                              bool accept_events) {
  // Initialize the popup.
  views::Widget* popup = new views::Widget;
  views::Widget::InitParams params(views::Widget::InitParams::TYPE_POPUP);
  params.opacity = views::Widget::InitParams::TRANSLUCENT_WINDOW;
  params.ownership = views::Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
  params.parent = parent_view;
  params.accept_events = accept_events;
  popup->Init(params);
  popup->SetContentsView(view);

  return popup;
}
