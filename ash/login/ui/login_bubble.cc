// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/login/ui/login_bubble.h"

#include "ash/resources/vector_icons/vector_icons.h"
#include "ash/shell.h"
#include "ui/gfx/font.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/focus/focus_manager.h"
#include "ui/views/layout/box_layout.h"
#include "ui/wm/core/coordinate_conversion.h"

namespace ash {
namespace {

const char* kLoginErrorBubbleViewName = "LoginErrorBubbleView";
const char* kLoginUserMenuViewName = "LoginUserMenuView";

// The size of the alert icon in the error bubble.
constexpr int kAlertIconSizeDp = 20;

// Vertical spacing with the anchor view.
constexpr int kAnchorViewVerticalSpacingDp = 48;

// An alpha value for the sub message in the user menu.
const SkAlpha kSubMessageColorAlpha = 0x89;

// Horizontal spacing with the anchor view.
constexpr int kAnchorViewHorizontalSpacingDp = 100;

views::Label* CreateLabel(const base::string16& message, SkColor color) {
  views::Label* label = new views::Label(message, views::style::CONTEXT_LABEL,
                                         views::style::STYLE_PRIMARY);
  label->SetAutoColorReadabilityEnabled(false);
  label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  label->SetEnabledColor(color);
  const gfx::FontList& base_font_list = views::Label::GetDefaultFontList();
  label->SetFontList(base_font_list.Derive(0, gfx::Font::FontStyle::NORMAL,
                                           gfx::Font::Weight::NORMAL));
  return label;
}

class LoginErrorBubbleView : public LoginBaseBubbleView {
 public:
  LoginErrorBubbleView(const base::string16& message, views::View* anchor_view)
      : LoginBaseBubbleView(anchor_view) {
    set_anchor_view_insets(gfx::Insets(kAnchorViewVerticalSpacingDp, 0));

    views::View* alert_view = new views::View();
    alert_view->SetLayoutManager(
        new views::BoxLayout(views::BoxLayout::kHorizontal, gfx::Insets()));
    views::ImageView* alert_icon = new views::ImageView();
    alert_icon->SetPreferredSize(gfx::Size(kAlertIconSizeDp, kAlertIconSizeDp));
    alert_icon->SetImage(
        gfx::CreateVectorIcon(kLockScreenAlertIcon, SK_ColorWHITE));
    alert_view->AddChildView(alert_icon);
    AddChildView(alert_view);

    views::Label* label = CreateLabel(message, SK_ColorWHITE);
    label->SetMultiLine(true);
    AddChildView(label);
  }

  ~LoginErrorBubbleView() override = default;

 private:
  // views::View:
  const char* GetClassName() const override {
    return kLoginErrorBubbleViewName;
  }

  DISALLOW_COPY_AND_ASSIGN(LoginErrorBubbleView);
};

class LoginUserMenuView : public LoginBaseBubbleView {
 public:
  LoginUserMenuView(const base::string16& message,
                    const base::string16& sub_message,
                    views::View* anchor_view)
      : LoginBaseBubbleView(anchor_view) {
    views::Label* label = CreateLabel(message, SK_ColorWHITE);
    views::Label* sub_label = CreateLabel(
        sub_message, SkColorSetA(SK_ColorWHITE, kSubMessageColorAlpha));
    AddChildView(label);
    AddChildView(sub_label);
    set_anchor_view_insets(gfx::Insets(0, kAnchorViewHorizontalSpacingDp));
  }

  ~LoginUserMenuView() override = default;

 private:
  // views::View:
  const char* GetClassName() const override { return kLoginUserMenuViewName; }

  DISALLOW_COPY_AND_ASSIGN(LoginUserMenuView);
};

}  // namespace

LoginBubble::LoginBubble(LoginBubbleType type, views::View* button_view)
    : type_(type), button_view_(button_view) {
  Shell::Get()->AddPreTargetHandler(this);
}

LoginBubble::~LoginBubble() {
  Shell::Get()->RemovePreTargetHandler(this);
  if (bubble_view_)
    bubble_view_->GetWidget()->RemoveObserver(this);
}

void LoginBubble::Show(const base::string16& message,
                       const base::string16& sub_message,
                       views::View* anchor_view) {
  if (type_ == LoginBubbleType::kErrorBubble) {
    bubble_view_ = new LoginErrorBubbleView(message, anchor_view);
  } else if (type_ == LoginBubbleType::kUserMenu) {
    bubble_view_ = new LoginUserMenuView(message, sub_message, anchor_view);
  }
  views::BubbleDialogDelegateView::CreateBubble(bubble_view_)->Show();
  bubble_view_->SetAlignment(views::BubbleBorder::ALIGN_EDGE_TO_ANCHOR_EDGE);
  bubble_view_->GetWidget()->AddObserver(this);
}

void LoginBubble::Close() {
  if (bubble_view_)
    bubble_view_->GetWidget()->Close();
}

bool LoginBubble::IsVisible() {
  return bubble_view_ && bubble_view_->GetWidget()->IsVisible();
}

void LoginBubble::OnWidgetClosing(views::Widget* widget) {
  widget->RemoveObserver(this);
  bubble_view_ = nullptr;
}

void LoginBubble::OnWidgetDestroying(views::Widget* widget) {
  widget->RemoveObserver(this);
  bubble_view_ = nullptr;
}

void LoginBubble::OnMouseEvent(ui::MouseEvent* event) {
  if (event->type() == ui::ET_MOUSE_PRESSED) {
    ProcessPressedEvent(event->location(),
                        static_cast<aura::Window*>(event->target()));
  }
}

void LoginBubble::OnGestureEvent(ui::GestureEvent* event) {
  if (event->type() == ui::ET_GESTURE_TAP ||
      event->type() == ui::ET_GESTURE_TAP_DOWN) {
    ProcessPressedEvent(event->location(),
                        static_cast<aura::Window*>(event->target()));
  }
}

void LoginBubble::OnKeyEvent(ui::KeyEvent* event) {
  if (!bubble_view_ || event->type() != ui::ET_KEY_PRESSED)
    return;

  // If current focus view is the button view, don't process the event here,
  // let the button logic handle the event and determine show/hide behavior.
  if (bubble_view_->anchor_widget() &&
      bubble_view_->anchor_widget()->GetFocusManager()) {
    views::View* focused_view =
        bubble_view_->anchor_widget()->GetFocusManager()->GetFocusedView();
    if (button_view_ && button_view_ == focused_view)
      return;
  }
  Close();
}

void LoginBubble::ProcessPressedEvent(const gfx::Point& location,
                                      gfx::NativeView target) {
  if (!bubble_view_)
    return;

  // Don't process event inside the bubble.
  gfx::Point screen_location = location;
  ::wm::ConvertPointToScreen(target, &screen_location);
  gfx::Rect bounds = bubble_view_->GetWidget()->GetWindowBoundsInScreen();
  if (bounds.Contains(screen_location))
    return;

  // If the user clicks on the button view, don't process the event here,
  // let the button logic handle the event and determine show/hide behavior.
  if (button_view_) {
    gfx::Rect button_bounds = button_view_->GetBoundsInScreen();
    if (button_bounds.Contains(screen_location))
      return;
  }
  Close();
}

}  // namespace ash
