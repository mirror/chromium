// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/omnibox/omnibox_popup_shadow_frame.h"

#include "ui/compositor/layer.h"
#include "ui/compositor/shadow_layer.h"
#include "ui/views/painter.h"

namespace {

constexpr int kCornerRadius = 8;
constexpr int kElevation = 16;

}  // namespace

OmniboxPopupShadowFrame::OmniboxPopupShadowFrame(views::View* contents)
    : shadow_details_(gfx::ShadowDetails::Get(kElevation, kCornerRadius)),
      border_insets_(GetBorderInsets()),
      contents_(contents) {
  // Host the contents in its own View to simplify layout and clipping.
  contents_host_ = new views::View();
  contents_host_->AddChildView(contents_);
  contents_host_->SetPaintToLayer();
  contents_host_->layer()->SetFillsBoundsOpaquely(false);

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

  AddChildView(shadow_host_);
  AddChildView(contents_host_);
}

OmniboxPopupShadowFrame::~OmniboxPopupShadowFrame() = default;

// static
gfx::Insets OmniboxPopupShadowFrame::GetBorderInsets() {
  return gfx::ShadowValue::GetBlurRegion(
             gfx::ShadowDetails::Get(kElevation, kCornerRadius).values) +
         gfx::Insets(kCornerRadius);
}

void OmniboxPopupShadowFrame::SetTargetBounds(const gfx::Rect& window_bounds) {
  gfx::Rect content_bounds = gfx::Rect(window_bounds.size());
  shadow_host_->SetSize(content_bounds.size());
  content_bounds.Inset(border_insets_);

  const gfx::Rect old_shadow_bounds = shadow_->content_bounds();
  if (old_shadow_bounds == content_bounds)
    return;

  contents_host_->SetBoundsRect(content_bounds);
  contents_mask_->layer()->SetBounds(gfx::Rect(content_bounds.size()));
  contents_->SetBoundsRect(gfx::Rect(content_bounds.size()));
  shadow_->SetContentBounds(content_bounds);
}

const char* OmniboxPopupShadowFrame::GetClassName() const {
  return "OmniboxPopupShadowFrame";
}

void OmniboxPopupShadowFrame::Layout() {
  gfx::Rect content_bounds = GetLocalBounds();
  content_bounds.Inset(border_insets_);

  // Update widths. Height is dynamic.
  contents_host_->SetBoundsRect(content_bounds);
  contents_mask_->layer()->SetBounds(gfx::Rect(content_bounds.size()));
  contents_->SetBoundsRect(gfx::Rect(content_bounds.size()));
  shadow_->SetContentBounds(content_bounds);
}
