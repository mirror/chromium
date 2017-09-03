// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_LOGIN_UI_LOGIN_BUBBLE_H_
#define ASH_LOGIN_UI_LOGIN_BUBBLE_H_

#include "ash/ash_export.h"
#include "ash/login/ui/login_base_bubble_view.h"
#include "base/strings/string16.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget_observer.h"

namespace ash {

// A wrapper for the bubble view in the login screen.
// This class observes keyboard events, mouse clicks and touch down events
// and dismisses the bubble accordingly.
class ASH_EXPORT LoginBubble : public views::WidgetObserver,
                               public ui::EventHandler {
 public:
  enum class LoginBubbleType { kErrorBubble, kUserMenu };

  LoginBubble(LoginBubbleType type, views::View* button_view);
  ~LoginBubble() override;

  // Show the bubble.
  void Show(const base::string16& message,
            const base::string16& sub_message,
            views::View* anchor_view);

  // Close the bubble.
  void Close();

  // True if the bubble is visible.
  bool IsVisible();

  // views::WidgetObservers:
  void OnWidgetClosing(views::Widget* widget) override;
  void OnWidgetDestroying(views::Widget* widget) override;

  // ui::EventHandler
  void OnMouseEvent(ui::MouseEvent* event) override;
  void OnGestureEvent(ui::GestureEvent* event) override;
  void OnKeyEvent(ui::KeyEvent* event) override;

  LoginBaseBubbleView* bubble_view() { return bubble_view_; }

  views::View* button_view() { return button_view_; }

 private:
  void ProcessPressedEvent(const gfx::Point& location, gfx::NativeView target);
  LoginBaseBubbleView* bubble_view_ = nullptr;
  LoginBubbleType type_;

  // Button that could open/close the bubble.
  views::View* button_view_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(LoginBubble);
};

}  // namespace ash

#endif  // ASH_LOGIN_UI_LOGIN_BUBBLE_H_
