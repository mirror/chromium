// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_TRY_CHROME_DIALOG_WIN_BUTTON_LAYOUT_H_
#define CHROME_BROWSER_UI_VIEWS_TRY_CHROME_DIALOG_WIN_BUTTON_LAYOUT_H_

#include "base/macros.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/layout/layout_manager.h"

namespace views {
class View;
}

// A LayoutManager that positions the TryChromeDialog's action button(s) based
// on the number and their width requirements. The possible layouts are:
//
// One narrow button:  |           [button 1]|
//
// Two narrow buttons: |[button 1] [button 2]|
//
// One wide button:    |[      button 1    ]|
//
// Two wide buttons:   |[      button 1    ]|
//                     |[      button 2    ]|
class ButtonLayout : public views::LayoutManager {
 public:
  // Returns a new instance that has been made the manager of |view|.
  // |view_width| is the desired width of the view, which controls the width of
  // the individual buttons as above.
  static ButtonLayout* CreateAndInstall(views::View* view, int view_width);

  ~ButtonLayout() override;

 protected:
  // views::LayoutManager:
  void Layout(views::View* host) override;
  gfx::Size GetPreferredSize(const views::View* host) const override;

 private:
  explicit ButtonLayout(int view_width);

  // Returns the size of the largest child of |view|.
  gfx::Size GetMaxChildSize(const views::View* host) const;

  // Returns true if wide buttons must be used based on the given widths.
  static bool UseWideButtons(int host_width, int max_child_width);

  // The desired width of the view.
  const int view_width_;

  DISALLOW_COPY_AND_ASSIGN(ButtonLayout);
};

#endif  // CHROME_BROWSER_UI_VIEWS_TRY_CHROME_DIALOG_WIN_BUTTON_LAYOUT_H_
