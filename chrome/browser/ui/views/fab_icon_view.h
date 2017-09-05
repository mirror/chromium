// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_FAB_ICON_VIEW_H_
#define CHROME_BROWSER_UI_VIEWS_FAB_ICON_VIEW_H_

#include "base/macros.h"
#include "ui/views/view.h"

namespace gfx {

struct VectorIcon;

}  // namespace gfx

namespace views {

class ImageView;
class Widget;

}  // namespace views

////////////////////////////////////////////////////////////////////////////////
//
// FabIconView is for showing a FAB-like UI, a white icon with a transparent-
// black circle background.
//
////////////////////////////////////////////////////////////////////////////////
class FabIconView : public views::View {
 public:
  FabIconView();
  ~FabIconView() override;

  // Sets the icon to be drawn on the view. Ideally it should be a 24x24 icon.
  void SetIcon(const gfx::VectorIcon& icon);

  // views::View overrides.
  void Layout() override;

  // Creates a Widget containing a FabIconView. If |accept_events|,
  // the bubble will intercept mouse events (required if there is a clickable
  // link); if not, events will go through to the underlying window.
  static views::Widget* CreatePopupWidget(gfx::NativeView parent_view,
                                          FabIconView* view,
                                          bool accept_events);

 private:
  views::ImageView* close_image_view_;

  DISALLOW_COPY_AND_ASSIGN(FabIconView);
};

#endif  // CHROME_BROWSER_UI_VIEWS_FAB_ICON_VIEW_H_
