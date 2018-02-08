// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_OMNIBOX_OMNIBOX_POPUP_ROUNDED_FRAME_H_
#define CHROME_BROWSER_UI_VIEWS_OMNIBOX_OMNIBOX_POPUP_ROUNDED_FRAME_H_

#include <memory>

#include "ui/gfx/geometry/insets.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

class LocationBarView;

// A class that wraps a Widget's content view to provide a custom popup frame.
class OmniboxPopupShadowFrame : public views::View {
 public:
  OmniboxPopupShadowFrame(views::View* contents, LocationBarView* location_bar);
  ~OmniboxPopupShadowFrame() override;

  // Hook to customize Widget initialization for the popup.
  static void OnBeforeWidgetInit(views::Widget::InitParams* params);

  // Insets applied to the browser location bar to determine where the location
  // and width of the top edge of the popup should appear.
  static gfx::Insets GetAlignmentInsets(views::View* location_bar);

  // views::View:
  const char* GetClassName() const override;
  void Layout() override;
  void AddedToWidget() override;

 private:
  const gfx::Insets content_insets_;

  std::unique_ptr<ui::LayerOwner> contents_mask_;

  views::View* contents_ = nullptr;
  views::View* contents_host_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(OmniboxPopupShadowFrame);
};

#endif  // CHROME_BROWSER_UI_VIEWS_OMNIBOX_OMNIBOX_POPUP_ROUNDED_FRAME_H_
