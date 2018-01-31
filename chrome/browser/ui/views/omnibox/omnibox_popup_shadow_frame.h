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

// A class that wraps a Widget's content view to provide a frame with a depth
// shadow, which can be animated as it grows and shrinks. The size of the Widget
// itself does not animate but is given a fixed height within which the shadow
// frame animates. The contents also does not animate, but can draw to any
// height: it will be clipped to fit within the shadow frame.
class OmniboxPopupShadowFrame : public views::View {
 public:
  explicit OmniboxPopupShadowFrame(views::View* contents);
  ~OmniboxPopupShadowFrame() override;

  static int GetWindowHeight();

  static gfx::Insets GetBorderInsets();

  // The minimum inset to avoid corners poking outside the rounded rectangle.
  // Technically constexpr, but that doesn't work for ceil/sqrt.
  static int GetContentOffset();

  void SetTargetBounds(const gfx::Rect& window_bounds);

  // views::View:
  const char* GetClassName() const override;
  void Layout() override;

 private:
  const gfx::ShadowDetails shadow_;
  const gfx::Insets insets_;
  const int content_offset_ = GetContentOffset();

  std::unique_ptr<ui::Shadow> contents_shadow_;
  views::View* contents_ = nullptr;
  views::View* clip_view_ = nullptr;
  views::View* background_host_ = nullptr;
  views::View* background_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(OmniboxPopupShadowFrame);
};

#endif  // CHROME_BROWSER_UI_VIEWS_OMNIBOX_OMNIBOX_POPUP_SHADOW_FRAME_H_
