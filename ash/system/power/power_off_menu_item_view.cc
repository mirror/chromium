// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/power/power_off_menu_item_view.h"

#include "ui/gfx/canvas.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/font.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"

namespace ash {

namespace {

constexpr SkColor kItemTitleColor = SkColorSetRGB(0x5F, 0x63, 0x68);

constexpr int kTitleFontSize = 14;
}  // namespace

PowerOffMenuItemView::PowerOffMenuItemView(views::ButtonListener* listener,
                                           const gfx::VectorIcon& icon,
                                           base::string16 title_text)
    : views::ImageButton(listener),
      icon_view_(new views::ImageView()),
      title_(new views::Label) {
  SetPaintToLayer();
  AddChildView(icon_view_);
  icon_view_->SetImage(gfx::CreateVectorIcon(icon, gfx::kChromeIconGrey));

  title_->SetBackgroundColor(SK_ColorTRANSPARENT);
  title_->SetAutoColorReadabilityEnabled(false);

  const gfx::FontList& font_list(gfx::Font("power_off", kTitleFontSize));
  title_->SetFontList(font_list);
  title_->SetEnabledColor(kItemTitleColor);
  title_->SetText(title_text);
  AddChildView(title_);
}

PowerOffMenuItemView::~PowerOffMenuItemView() = default;

void PowerOffMenuItemView::Layout() {
  gfx::Rect rect(GetContentsBounds());

  gfx::Rect icon_rect(rect);
  icon_rect.ClampToCenteredSize(gfx::Size(24, 24));
  icon_rect.set_y(16);
  icon_view_->SetBoundsRect(icon_rect);

  gfx::Rect title_rect(rect);
  title_rect.ClampToCenteredSize(
      gfx::Size(96, title_->font_list().GetHeight()));
  title_rect.set_y(50);
  title_->SetBoundsRect(title_rect);
}

void PowerOffMenuItemView::PaintButtonContents(gfx::Canvas* canvas) {
  views::View::OnPaint(canvas);
  canvas->DrawColor(SK_ColorWHITE);
}

}  // namespace ash
