
// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_FRAME_NATIVE_NAV_BUTTONS_FRAME_VIEW_H_
#define CHROME_BROWSER_UI_VIEWS_FRAME_NATIVE_NAV_BUTTONS_FRAME_VIEW_H_

#include "chrome/browser/ui/views/frame/opaque_browser_frame_view.h"

class NativeNavButtonsFrameView : public OpaqueBrowserFrameView {
 public:
  NativeNavButtonsFrameView(
      BrowserFrame* frame,
      BrowserView* browser_view,
      OpaqueBrowserFrameViewLayout* layout,
      std::unique_ptr<views::NavButtonProvider> nav_button_provider);
  ~NativeNavButtonsFrameView() override;

 protected:
  // OpaqueBrowserFrameView:
  void MaybeRedrawFrameButtons() override;

 private:
  // Returns one of |{minimize,maximize,restore,close}_button_|
  // corresponding to |type|.
  views::ImageButton* GetButtonFromDisplayType(
      chrome::FrameButtonDisplayType type);

  std::unique_ptr<views::NavButtonProvider> nav_button_provider_;
};

#endif  // CHROME_BROWSER_UI_VIEWS_FRAME_NATIVE_NAV_BUTTONS_FRAME_VIEW_H_
