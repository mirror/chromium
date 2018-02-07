// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_OMNIBOX_OMNIBOX_POPUP_SHADOW_FRAME_H_
#define CHROME_BROWSER_UI_VIEWS_OMNIBOX_OMNIBOX_POPUP_SHADOW_FRAME_H_

#include <cmath>
#include <memory>

#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/shadow_util.h"
#include "ui/views/view.h"

namespace ui {
class Shadow;
}

class LocationBarView;

// A class that wraps a Widget's content view to provide a frame with a depth
// shadow, which can be animated as it grows and shrinks. The size of the Widget
// itself does not animate but is given a fixed height within which the shadow
// frame animates. The contents also does not animate, but can draw to any
// height: it will be clipped to fit within the shadow frame.
class OmniboxPopupShadowFrame : public views::View {
 public:
  // From the spec. The popup must be positioned such that the LocationBarView
  // is below the popup at this offset.
  static constexpr gfx::Insets kPopupLocationBarAlignmentInsets =
      gfx::Insets(6, 6, 0, 6);

  OmniboxPopupShadowFrame(views::View* contents, LocationBarView* location_bar);
  ~OmniboxPopupShadowFrame() override;

  static gfx::Insets GetTotalContentInsets(views::View* location_bar);

  void SetTargetBounds(const gfx::Rect& window_bounds);

  // views::View:
  const char* GetClassName() const override;
  void Layout() override;

 private:
  static gfx::Insets GetShadowInsets();
  static gfx::Insets GetContentInsets(views::View* location_bar);

  const gfx::Insets shadow_insets_;
  const gfx::Insets content_insets_;

  std::unique_ptr<ui::Shadow> shadow_;
  std::unique_ptr<ui::LayerOwner> contents_mask_;

  views::View* contents_ = nullptr;
  views::View* contents_host_ = nullptr;
  views::View* shadow_host_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(OmniboxPopupShadowFrame);
};

#endif  // CHROME_BROWSER_UI_VIEWS_OMNIBOX_OMNIBOX_POPUP_SHADOW_FRAME_H_
