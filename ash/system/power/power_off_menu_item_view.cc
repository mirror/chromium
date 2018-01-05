// Copyright 2018 The Chromium Authors. All rights reserved.
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
constexpr int kIconSize = 24;
constexpr int kIconTopPadding = 16;
constexpr int kTitleTopPadding = 50;

}  // namespace

PowerOffMenuItemView::PowerOffMenuItemView(views::ButtonListener* listener,
                                           const gfx::VectorIcon& icon,
                                           base::string16 title_text)
    : views::ImageButton(listener),
      icon_view_(new views::ImageView()),
      title_(new views::Label) {
  SetPaintToLayer();
  icon_view_->SetImage(gfx::CreateVectorIcon(icon, gfx::kChromeIconGrey));
  AddChildView(icon_view_);

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
  icon_rect.ClampToCenteredSize(gfx::Size(kIconSize, kIconSize));
  icon_rect.set_y(kIconTopPadding);
  icon_view_->SetBoundsRect(icon_rect);

  gfx::Rect title_rect(rect);
  title_rect.ClampToCenteredSize(
      gfx::Size(kMenuItemWidth, title_->font_list().GetHeight()));
  title_rect.set_y(kTitleTopPadding);
  title_->SetBoundsRect(title_rect);
}

gfx::Size PowerOffMenuItemView::CalculatePreferredSize() const {
  return gfx::Size(kMenuItemWidth, kMenuItemHeight);
}

void PowerOffMenuItemView::PaintButtonContents(gfx::Canvas* canvas) {
  views::View::OnPaint(canvas);
  canvas->DrawColor(SK_ColorWHITE);
}

}  // namespace ash
