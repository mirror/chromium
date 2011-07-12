// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef VIEWS_DESKTOP_DESKTOP_WINDOW_H_
#define VIEWS_DESKTOP_DESKTOP_WINDOW_H_

#include "views/view.h"
#include "views/widget/widget_delegate.h"

namespace views {
class NativeWidgetViews;

namespace desktop {

class DesktopWindowView : public WidgetDelegateView {
 public:
  static DesktopWindowView* desktop_window_view;

  DesktopWindowView();
  virtual ~DesktopWindowView();

  static void CreateDesktopWindow();

  // Changes activation to the specified Widget. The currently active Widget
  // is de-activated.
  void ActivateWidget(Widget* widget);

  void CreateTestWindow(const std::wstring& title,
                        SkColor color,
                        gfx::Rect initial_bounds,
                        bool rotate);

 private:
  // Overridden from View:
  virtual void Layout() OVERRIDE;

  // Overridden from WidgetDelegate:
  virtual bool CanResize() const OVERRIDE;
  virtual bool CanMaximize() const OVERRIDE;
  virtual std::wstring GetWindowTitle() const OVERRIDE;
  virtual SkBitmap GetWindowAppIcon() OVERRIDE;
  virtual SkBitmap GetWindowIcon() OVERRIDE;
  virtual bool ShouldShowWindowIcon() const OVERRIDE;
  virtual void WindowClosing() OVERRIDE;
  virtual View* GetContentsView() OVERRIDE;

  NativeWidgetViews* active_widget_;

  DISALLOW_COPY_AND_ASSIGN(DesktopWindowView);
};

}  // namespace desktop
}  // namespace views

#endif  // VIEWS_DESKTOP_DESKTOP_WINDOW_H_
