// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/omnibox/omnibox_popup_shadow_frame.h"

#include "chrome/browser/ui/views/location_bar/background_with_1_px_border.h"
#include "chrome/browser/ui/views/location_bar/location_bar_view.h"
#include "ui/compositor/layer.h"
#include "ui/compositor/shadow_layer.h"
#include "ui/views/painter.h"
#include "ui/views/widget/widget.h"

namespace {

constexpr int kCornerRadius = 8;
constexpr int kElevation = 16;

}  // namespace

constexpr gfx::Insets OmniboxPopupShadowFrame::kPopupLocationBarAlignmentInsets;

OmniboxPopupShadowFrame::OmniboxPopupShadowFrame(views::View* contents,
                                                 LocationBarView* location_bar)
    : shadow_insets_(GetShadowInsets()),
      content_insets_(GetContentInsets(location_bar)),
      contents_(contents) {
  // Host the contents in its own View to simplify layout and clipping.
  contents_host_ = new views::View();
  contents_host_->AddChildView(contents_);
  contents_host_->SetPaintToLayer();
  contents_host_->layer()->SetFillsBoundsOpaquely(false);

  SkColor background_color = GetNativeTheme()->GetSystemColor(
      ui::NativeTheme::kColorId_ResultsTableNormalBackground);
  contents_host_->SetBackground(views::CreateSolidBackground(background_color));

  // Use a textured mask to clip contents. This doesn't work on Windows
  // (https://crbug.com/713359), and can't be animated, but it simplifies
  // selection highlights.
  // TODO(tapted): Remove this and have the contents paint a half-rounded rect
  // for the background and when selecting the bottom row.
  contents_mask_ = views::Painter::CreatePaintedLayer(
      views::Painter::CreateSolidRoundRectPainter(SK_ColorBLACK,
                                                  kCornerRadius));
  contents_mask_->layer()->SetFillsBoundsOpaquely(false);
  contents_host_->layer()->SetMaskLayer(contents_mask_->layer());

  shadow_ = std::make_unique<ui::Shadow>();
  shadow_->SetRoundedCornerRadius(kCornerRadius);
  shadow_->Init(kElevation);

  shadow_host_ = new views::View();
  shadow_host_->SetPaintToLayer(ui::LAYER_NOT_DRAWN);
  shadow_host_->layer()->Add(shadow_->layer());

  // Paint the omnibox border with transparent pixels to make a hole.
  views::View* top_background = new views::View();
  auto background = std::make_unique<BackgroundWith1PxBorder>(
      SK_ColorTRANSPARENT, background_color);
  background->set_blend_mode(SkBlendMode::kSrc);
  top_background->SetBackground(std::move(background));
  gfx::Size location_bar_size = location_bar->bounds().size();
  top_background->SetBounds(kPopupLocationBarAlignmentInsets.left(),
                            kPopupLocationBarAlignmentInsets.top(),
                            location_bar_size.width(),
                            location_bar_size.height());

  contents_host_->AddChildView(top_background);

  AddChildView(shadow_host_);
  AddChildView(contents_host_);
}

OmniboxPopupShadowFrame::~OmniboxPopupShadowFrame() = default;

// static
gfx::Insets OmniboxPopupShadowFrame::GetTotalContentInsets(
    views::View* location_bar) {
  return GetShadowInsets() + kPopupLocationBarAlignmentInsets +
         gfx::Insets(0, 0, location_bar->height(), 0);
}

void OmniboxPopupShadowFrame::SetTargetBounds(const gfx::Rect& window_bounds) {
  // Set the Widget bounds and do layout. This is fast on ChromeOS, but slow on
  // other platforms, and can't be animated.
  // TODO(tapted): Investigate using a static Widget size.
  GetWidget()->SetBounds(window_bounds);
}

const char* OmniboxPopupShadowFrame::GetClassName() const {
  return "OmniboxPopupShadowFrame";
}

void OmniboxPopupShadowFrame::Layout() {
  gfx::Rect bounds = GetLocalBounds();
  bounds.Inset(shadow_insets_);
  shadow_->SetContentBounds(bounds);
  contents_host_->SetBoundsRect(bounds);
  contents_mask_->layer()->SetBounds(gfx::Rect(bounds.size()));

  gfx::Rect results_bounds = gfx::Rect(bounds.size());
  results_bounds.Inset(content_insets_);
  contents_->SetBoundsRect(results_bounds);
}

// static
gfx::Insets OmniboxPopupShadowFrame::GetShadowInsets() {
  return gfx::ShadowValue::GetBlurRegion(
      gfx::ShadowDetails::Get(kElevation, kCornerRadius).values);
}

// static
gfx::Insets OmniboxPopupShadowFrame::GetContentInsets(
    views::View* location_bar) {
  // TODO(tapted): When icon alignment is fixed, this should be changed to only
  // use kPopupLocationBarAlignmentInsets.top() so that the selection highlight
  // goes to the edges of the popup.
  return kPopupLocationBarAlignmentInsets +
         gfx::Insets(location_bar->height(), 0, 0, 0);
}
