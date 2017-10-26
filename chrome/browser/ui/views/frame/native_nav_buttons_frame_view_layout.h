// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_FRAME_NATIVE_NAV_BUTTONS_FRAME_VIEW_LAYOUT_H_
#define CHROME_BROWSER_UI_VIEWS_FRAME_NATIVE_NAV_BUTTONS_FRAME_VIEW_LAYOUT_H_

#include "chrome/browser/ui/views/frame/opaque_browser_frame_view_layout.h"

class NativeNavButtonsFrameViewLayout : public OpaqueBrowserFrameViewLayout {
 public:
  NativeNavButtonsFrameViewLayout(
      views::NavButtonProvider* nav_button_provider);

  // OpaqueBrowserFrameViewLayout:
  int CaptionButtonY(chrome::FrameButtonDisplayType button_id,
                     bool restored) const override;
  int TopAreaPadding() const override;
  int GetWindowCaptionSpacing(views::FrameButton button_id,
                              bool leading_spacing,
                              bool is_leading_button) const override;
  void LayoutNewStyleAvatar(views::View* host) override;
  bool ShouldDrawImageMirrored(views::ImageButton* button,
                               ButtonAlignment alignment) const override;

 private:
  views::NavButtonProvider* nav_button_provider_;
};

#endif  // CHROME_BROWSER_UI_VIEWS_FRAME_NATIVE_NAV_BUTTONS_FRAME_VIEW_LAYOUT_H_
