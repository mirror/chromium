// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/fullscreen_control_view.h"

#include <memory>

#include "base/memory/ptr_util.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/views/background.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/layout/box_layout.h"

FullscreenControlView::FullscreenControlView(
    BrowserView* browser_view,
    FullscreenControlHost* fullscreen_control_host)
    : browser_view_(browser_view),
      fullscreen_control_host_(fullscreen_control_host),
      exit_fullscreen_(new views::LabelButton(this, L"Exit Fullscreen")) {
  AddChildView(exit_fullscreen_);
  SetLayoutManager(
      new views::BoxLayout(views::BoxLayout::kHorizontal, gfx::Insets(10), 10));
  SetBackground(views::CreateSolidBackground(SkColorSetRGB(0xfe, 0xfe, 0xfe)));
}

FullscreenControlView::~FullscreenControlView() = default;

void FullscreenControlView::SetFocusAndSelection(bool select_all) {}

void FullscreenControlView::ButtonPressed(views::Button* sender,
                                          const ui::Event& event) {
  if (sender == exit_fullscreen_)
    browser_view_->ExitFullscreen();
}
