// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/chromeos/ksv/views/bubble_view.h"

#include <memory>

#include "cc/paint/paint_flags.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/gfx/shadow_value.h"
#include "ui/gfx/skia_paint_util.h"
#include "ui/views/border.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/fill_layout.h"

namespace keyboard_shortcut_viewer {

namespace {

// Preferred padding.
constexpr int kInnerPadding = 6;

constexpr int kCornerRadius = 22;

constexpr SkColor kBackgroundColor = SkColorSetRGB(0xE8, 0xEA, 0xED);

constexpr SkColor kKeyColor = SkColorSetRGB(0x5F, 0x63, 0x68);

constexpr SkColor kShadowColor = SkColorSetARGB(0x15, 0, 0, 0);
constexpr int kShadowBlur = 4;
constexpr int kShadowXOffset = 0;
constexpr int kShadowYOffset = 2;

// Preferred icon size.
constexpr int kIconSize = 20;

}  // namespace

BubbleView::BubbleView()
    : shadows_({gfx::ShadowValue(gfx::Vector2d(kShadowXOffset, kShadowYOffset),
                                 kShadowBlur,
                                 kShadowColor)}) {
  SetLayoutManager(std::make_unique<views::FillLayout>());
  SetBorder(views::CreateEmptyBorder(gfx::Insets(kInnerPadding)));
}

BubbleView::~BubbleView() = default;

void BubbleView::SetIcon(const gfx::VectorIcon& icon) {
  DCHECK(!text_);
  if (!icon_) {
    icon_ = new views::ImageView();
    AddChildView(icon_);
  }

  icon_->SetImage(gfx::CreateVectorIcon(icon, kKeyColor));
  icon_->SetImageSize(gfx::Size(kIconSize, kIconSize));
}

void BubbleView::SetText(const base::string16& text) {
  DCHECK(!icon_);
  if (!text_) {
    text_ = new views::Label();
    text_->SetEnabledColor(kKeyColor);
    text_->SetElideBehavior(gfx::NO_ELIDE);
    AddChildView(text_);
  }
  text_->SetText(text);
}

void BubbleView::OnPaint(gfx::Canvas* canvas) {
  views::View::OnPaint(canvas);

  cc::PaintFlags flags;
  flags.setLooper(gfx::CreateShadowDrawLooper(shadows_));
  flags.setColor(kBackgroundColor);
  flags.setStrokeJoin(cc::PaintFlags::kRound_Join);
  flags.setAntiAlias(true);

  gfx::Rect bounds(size());
  bounds.Inset(-gfx::ShadowValue::GetMargin(shadows_));
  canvas->DrawRoundRect(bounds, kCornerRadius, flags);
}

gfx::Size BubbleView::CalculatePreferredSize() const {
  gfx::Size size(kCornerRadius, kCornerRadius);
  if (text_)
    size.SetToMax(text_->GetPreferredSize());
  if (icon_)
    size.SetToMax(icon_->GetPreferredSize());
  size.Enlarge(GetInsets().width(), GetInsets().height());
  return size;
}

}  // namespace keyboard_shortcut_viewer
