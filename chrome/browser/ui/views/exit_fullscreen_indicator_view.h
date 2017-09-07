// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_EXIT_FULLSCREEN_INDICATOR_VIEW_H_
#define CHROME_BROWSER_UI_VIEWS_EXIT_FULLSCREEN_INDICATOR_VIEW_H_

#include "base/macros.h"
#include "ui/views/view.h"

namespace views {

class ImageView;

}  // namespace views

////////////////////////////////////////////////////////////////////////////////
//
// ExitFullscreenIndicatorView shows a partially-transparent black circle with
// a "X" icon in the middle. It's similar to the FAB (floating action button)
// from the material design spec.
//
////////////////////////////////////////////////////////////////////////////////
class ExitFullscreenIndicatorView : public views::View {
 public:
  ExitFullscreenIndicatorView();
  ~ExitFullscreenIndicatorView() override;

  // views::View overrides.
  void Layout() override;

 private:
  views::ImageView* close_image_view_;

  DISALLOW_COPY_AND_ASSIGN(ExitFullscreenIndicatorView);
};

#endif  // CHROME_BROWSER_UI_VIEWS_EXIT_FULLSCREEN_INDICATOR_VIEW_H_
